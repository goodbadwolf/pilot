#ifndef beams_rendering_transmittance_map_h
#define beams_rendering_transmittance_map_h

#include "../Intersections.h"
#include "LightRayOperations.h"
#include "LightRays.h"
#include "utils/Fmt.h"

#include "Lights.h"
#include <vtkm/Swap.h>
#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/exec/CellInterpolate.h>
#include <vtkm/exec/ParametricCoordinates.h>
#include <vtkm/io/VTKDataSetWriter.h>

#include <iostream>

namespace beams
{
namespace rendering
{
struct TransmittanceRayBlockHit
{
  int RayId;
  int BlockId;
  int FromBlockId;
  vtkm::Vec3f_32 Point;
  vtkm::Float32 RayT;
  vtkm::Float32 Opacity;
};

template <typename Device>
class TransmittanceLocator
{
public:
  using PointsArrayHandle = vtkm::cont::ArrayHandleUniformPointCoordinates;
  using PointsReadPortal = typename PointsArrayHandle::ReadPortalType;

  TransmittanceLocator(const PointsArrayHandle& coordinates,
                       const vtkm::Id3& pointDimensions,
                       vtkm::cont::Token& token)
    : Coordinates(coordinates.PrepareForInput(Device(), token))
    , PointDimensions(pointDimensions)
  {
    Origin = Coordinates.GetOrigin();
    CellDimensions = PointDimensions - vtkm::Id3{ 1, 1, 1 };
    vtkm::Vec3f_32 spacing = Coordinates.GetSpacing();

    vtkm::Vec3f_32 unitLength;
    unitLength[0] = static_cast<vtkm::Float32>(PointDimensions[0] - 1);
    unitLength[1] = static_cast<vtkm::Float32>(PointDimensions[1] - 1);
    unitLength[2] = static_cast<vtkm::Float32>(PointDimensions[2] - 1);
    MaxPoint = Origin + spacing * unitLength;
    InvSpacing[0] = 1.f / spacing[0];
    InvSpacing[1] = 1.f / spacing[1];
    InvSpacing[2] = 1.f / spacing[2];
  }

  VTKM_EXEC
  inline bool IsInside(const vtkm::Vec3f_32& point) const
  {
    bool inside = true;
    if (point[0] < Origin[0] || point[0] > MaxPoint[0])
      inside = false;
    if (point[1] < Origin[1] || point[1] > MaxPoint[1])
      inside = false;
    if (point[2] < Origin[2] || point[2] > MaxPoint[2])
      inside = false;
    return inside;
  }

  VTKM_EXEC
  inline void GetCellIndices(const vtkm::Id3& cell, vtkm::Vec<vtkm::Id, 8>& cellIndices) const
  {
    cellIndices[0] = (cell[2] * PointDimensions[1] + cell[1]) * PointDimensions[0] + cell[0];
    cellIndices[1] = cellIndices[0] + 1;
    cellIndices[2] = cellIndices[1] + PointDimensions[0];
    cellIndices[3] = cellIndices[2] - 1;
    cellIndices[4] = cellIndices[0] + PointDimensions[0] * PointDimensions[1];
    cellIndices[5] = cellIndices[4] + 1;
    cellIndices[6] = cellIndices[5] + PointDimensions[0];
    cellIndices[7] = cellIndices[6] - 1;
  } // GetCellIndices

  VTKM_EXEC
  inline vtkm::Id GetCellIndex(const vtkm::Id3& cell) const
  {
    return (cell[2] * (PointDimensions[1] - 1) + cell[1]) * (PointDimensions[0] - 1) + cell[0];
  }

  VTKM_EXEC
  inline void LocateCell(vtkm::Id3& cell, const vtkm::Vec3f_32& point) const
  {
    vtkm::Vec3f_32 temp = point;
    temp = temp - Origin;
    temp = temp * InvSpacing;
    //make sure that if we border the edges, we sample the correct cell
    if (temp[0] < 0.0f)
      temp[0] = 0.0f;
    if (temp[1] < 0.0f)
      temp[1] = 0.0f;
    if (temp[2] < 0.0f)
      temp[2] = 0.0f;
    if (temp[0] >= vtkm::Float32(PointDimensions[0] - 1))
      temp[0] = vtkm::Float32(PointDimensions[0] - 2);
    if (temp[1] >= vtkm::Float32(PointDimensions[1] - 1))
      temp[1] = vtkm::Float32(PointDimensions[1] - 2);
    if (temp[2] >= vtkm::Float32(PointDimensions[2] - 1))
      temp[2] = vtkm::Float32(PointDimensions[2] - 2);
    cell = temp;
  }

  VTKM_EXEC
  inline void GetPoint(const vtkm::Id& index, vtkm::Vec3f_32& point) const
  {
    BOUNDS_CHECK(Coordinates, index);
    point = Coordinates.Get(index);
  }

  VTKM_EXEC
  inline void GetMinPoint(const vtkm::Id3& cell, vtkm::Vec3f_32& point) const
  {
    const vtkm::Id pointIndex =
      (cell[2] * PointDimensions[1] + cell[1]) * PointDimensions[0] + cell[0];
    point = Coordinates.Get(pointIndex);
  }

protected:
  PointsReadPortal Coordinates;
  vtkm::Id3 PointDimensions;
  vtkm::Id3 CellDimensions;
  vtkm::Vec3f_32 Origin;
  vtkm::Vec3f_32 InvSpacing;
  vtkm::Vec3f_32 MaxPoint;
}; // class UniformLocator

template <typename Device, typename LocatorType>
struct TransmittanceMapEstimator
{
  using PointsArrayHandle = typename LocatorType::PointsArrayHandle;
  using PointsReadPortal = typename PointsArrayHandle::ReadPortalType;
  using TransmittanceArrayHandle = vtkm::cont::ArrayHandle<vtkm::Float32>;
  using TransmittanceReadPortal = typename TransmittanceArrayHandle::ReadPortalType;

  PointsArrayHandle LocationsHandle;
  PointsReadPortal Locations;
  TransmittanceArrayHandle TransmittancesHandle;
  TransmittanceReadPortal Transmittances;
  LocatorType Locator;
  vtkm::Vec3f_32 LightColor;

  TransmittanceMapEstimator(const PointsArrayHandle& locations,
                            const TransmittanceArrayHandle& transmittances,
                            const LocatorType& locator,
                            const vtkm::Vec3f_32 lightColor,
                            vtkm::cont::Token& token)
    : LocationsHandle(locations)
    , Locations(locations.PrepareForInput(Device(), token))
    , TransmittancesHandle(transmittances)
    , Transmittances(transmittances.PrepareForInput(Device(), token))
    , Locator(locator)
    , LightColor(lightColor)
  {
  }

  VTKM_EXEC
  inline vtkm::Vec3f GetEstimateUsingVertices(const vtkm::Vec3f& point) const
  {
    vtkm::Id3 cell;
    this->Locator.LocateCell(cell, point);

    if (!this->IsValidCell(this->Locator.GetCellIndex(cell)))
    {
      return { 1.0f, 1.0f, 1.0f };
    }
    vtkm::Vec<vtkm::Id, 8> cellIndices;
    this->Locator.GetCellIndices(cell, cellIndices);

    vtkm::Vec3f minPoint = this->Locations.Get(cellIndices[0]);
    vtkm::Vec3f maxPoint = this->Locations.Get(cellIndices[6]);

    vtkm::VecAxisAlignedPointCoordinates<3> rPoints(minPoint, maxPoint - minPoint);

    vtkm::Vec3f pcoords;
    vtkm::exec::WorldCoordinatesToParametricCoordinates(
      rPoints, point, vtkm::CellShapeTagHexahedron(), pcoords);

    vtkm::Vec<vtkm::FloatDefault, 8> scalars;
    for (vtkm::Id i = 0; i < 8; ++i)
    {
      scalars[i] = this->Transmittances.Get(cellIndices[i]);
    }
    vtkm::FloatDefault opacity;
    vtkm::exec::CellInterpolate(scalars, pcoords, vtkm::CellShapeTagHexahedron(), opacity);
    vtkm::Float32 transmittance = 1.0f - opacity;
    return transmittance * this->LightColor;
  }

  VTKM_EXEC
  inline vtkm::Float32 GetEstimateUsingVerticesT(const vtkm::Vec3f& point) const
  {
    vtkm::Id3 cell;
    this->Locator.LocateCell(cell, point);
    if (!this->IsValidCell(this->Locator.GetCellIndex(cell)))
    {
      return 1.0f;
    }
    vtkm::Vec<vtkm::Id, 8> cellIndices;
    this->Locator.GetCellIndices(cell, cellIndices);

    vtkm::Vec3f minPoint = this->Locations.Get(cellIndices[0]);
    vtkm::Vec3f maxPoint = this->Locations.Get(cellIndices[6]);

    vtkm::VecAxisAlignedPointCoordinates<3> rPoints(minPoint, maxPoint - minPoint);

    vtkm::Vec3f pcoords;
    vtkm::exec::WorldCoordinatesToParametricCoordinates(
      rPoints, point, vtkm::CellShapeTagHexahedron(), pcoords);

    vtkm::Vec<vtkm::FloatDefault, 8> scalars;
    for (vtkm::Id i = 0; i < 8; ++i)
    {
      scalars[i] = this->Transmittances.Get(cellIndices[i]);
    }
    vtkm::FloatDefault opacity;
    vtkm::exec::CellInterpolate(scalars, pcoords, vtkm::CellShapeTagHexahedron(), opacity);

    return opacity;
  }


  VTKM_EXEC
  inline bool IsValidCell(vtkm::Id cellId) const
  {
    return cellId >= 0 && cellId < Locations.GetNumberOfValues();
  }
};

struct TransmittanceMapGenerator : public vtkm::worklet::WorkletMapField
{
  VTKM_CONT
  TransmittanceMapGenerator(const vtkm::Float32& stepSize,
                            const vtkm::Float32& minScalar,
                            const vtkm::Float32& maxScalar,
                            const vtkm::Float32& maxDensity,
                            const vtkm::Vec3f& lightLoc,
                            const vtkm::Bounds& mapBounds)
    : StepSize(stepSize)
    , MinScalar(minScalar)
    , MaxDensity(maxDensity)
    , LightLoc(lightLoc)
    , MapBounds(mapBounds)
  {
    if ((maxScalar - minScalar) != 0.0f)
    {
      InverseDeltaScalar = 1.0f / (maxScalar - minScalar);
    }
    else
    {
      InverseDeltaScalar = minScalar;
    }
  }

  using ControlSignature = void(FieldIn rayIds,
                                FieldIn rayOrigins,
                                FieldIn rayDirs,
                                FieldIn rayDests,
                                ExecObject lights,
                                ExecObject meshOracle,
                                WholeArrayIn scalars,
                                WholeArrayIn colorMap,
                                FieldInOut opacities);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7, _8, _9);

  template <typename LightsType,
            typename OracleType,
            typename ScalarPortalType,
            typename ColorMapType>
  VTKM_EXEC void operator()(const vtkm::Id& vtkmNotUsed(rayId),
                            const vtkm::Vec3f& rayOrigin,
                            const vtkm::Vec3f& rayDir,
                            const vtkm::Vec3f& rayDest,
                            const LightsType& vtkmNotUsed(lights),
                            const OracleType& oracle,
                            ScalarPortalType& scalars,
                            const ColorMapType& colorMap,
                            vtkm::Float32& opacity) const
  {
    const vtkm::Id colorMapSize = colorMap.GetNumberOfValues() - 1;
    vtkm::Id cellId = -1;

    vtkm::Float32 tMin, tMax;
    bool hits = beams::Intersections::SegmentAABB(rayOrigin, rayDest, this->MapBounds, tMin, tMax);
    (void)hits;

    auto start = rayOrigin + tMin * rayDir;
    auto sampleLocation = start;
    vtkm::Float32 lengthSquared = (tMax - tMin) * (tMax - tMin);
    while (vtkm::MagnitudeSquared(sampleLocation - start) <= lengthSquared)
    {
      sampleLocation += StepSize * rayDir;

      vtkm::Vec<vtkm::Float32, 3> pcoords;
      oracle.FindCell(sampleLocation, cellId, pcoords);
      if (cellId == -1)
      {
        break;
      }
      vtkm::Id cellIndices[8];
      vtkm::Vec<vtkm::Float32, 8> values;
      vtkm::Float32 scalar = 0.f;

      const vtkm::Int32 numIndices = oracle.GetCellIndices(cellIndices, cellId);
      for (vtkm::Int32 i = 0; i < numIndices; ++i)
      {
        vtkm::Id j = cellIndices[i];
        j = (j >= scalars.GetNumberOfValues()) ? (scalars.GetNumberOfValues() - 1) : j;
        values[i] = static_cast<vtkm::Float32>(scalars.Get(j));
      }

      scalar = oracle.Interpolate(values, pcoords);
      scalar = (scalar - MinScalar) * InverseDeltaScalar;
      vtkm::Id colorIndex =
        static_cast<vtkm::Id>(scalar * static_cast<vtkm::Float32>(colorMapSize));
      constexpr vtkm::Id zero = 0;
      colorIndex = vtkm::Max(zero, vtkm::Min(colorMapSize, colorIndex));
      vtkm::Vec<vtkm::Float32, 4> sampleColor = colorMap.Get(colorIndex);

      vtkm::Float32 localOpacity = sampleColor[3] / MaxDensity;
      opacity = opacity + (1.0f - opacity) * localOpacity;
    }
  }

  const vtkm::Float32 StepSize;
  vtkm::Float32 MinScalar;
  vtkm::Float32 InverseDeltaScalar;
  vtkm::Float32 MaxDensity;
  vtkm::Vec3f LightLoc;
  vtkm::Bounds MapBounds;
};

struct CountNonLocalBlockHits : public vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn samplePoints, ExecObject boundMap, FieldOut numBlocks);
  using ExecutionSignature = void(_1, _2, _3);

  VTKM_CONT
  CountNonLocalBlockHits(const vtkm::Id& selfBlockId,
                         const vtkm::Vec3f& lightLoc,
                         bool useGlancingHits)
    : SelfBlockId(selfBlockId)
    , LightLocation(lightLoc)
    , UseGlancingHits(useGlancingHits)
  {
  }
  template <typename BoundsMapExec>
  VTKM_EXEC void operator()(const vtkm::Vec3f& samplePoint,
                            const BoundsMapExec& boundsMap,
                            vtkm::Id& numBlocks) const
  {
    vtkm::Vec3f origin = this->LightLocation;
    numBlocks = boundsMap.FindNumSegmentBlockIntersections(
      origin, samplePoint, this->UseGlancingHits, this->SelfBlockId);
  }

  vtkm::Id SelfBlockId;
  vtkm::Vec3f LightLocation;
  bool UseGlancingHits;
};

struct HitSort
{
  VTKM_EXEC_CONT bool operator()(const TransmittanceRayBlockHit& h1,
                                 const TransmittanceRayBlockHit& h2) const
  {
    if (h1.RayId == h2.RayId)
    {
      return h1.RayT < h2.RayT;
    }
    else
    {
      return h1.RayId < h2.RayId;
    }
  }
};

struct CalculateNonLocalBlockHits : public vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn samplePoints,
                                ExecObject boundMap,
                                FieldIn hitOffsets,
                                WholeArrayInOut hits);
  using ExecutionSignature = void(InputIndex, _1 samplePoint, _2 boundsMap, _3 hitOffset, _4 hits);

  VTKM_CONT
  CalculateNonLocalBlockHits(const vtkm::Id& selfBlockId,
                             const vtkm::Id& numBlocks,
                             const vtkm::Vec3f& lightLoc,
                             bool useGlancingHits)
    : SelfBlockId(selfBlockId)
    , NumBlocks(numBlocks)
    , LightLoc(lightLoc)
    , UseGlancingHits(useGlancingHits)
  {
  }

  template <typename BoundsMapExec, typename HitsPortal>
  VTKM_EXEC void operator()(vtkm::Id inputIndex,
                            const vtkm::Vec3f& samplePoint,
                            const BoundsMapExec& boundsMap,
                            const vtkm::Id& offset,
                            HitsPortal& hits) const
  {
    vtkm::Id rayId = inputIndex;
    vtkm::Vec3f origin = LightLoc;
    vtkm::Vec3f dir = samplePoint - origin;
    vtkm::Normalize(dir);

    vtkm::Float32 tMin, tMax;

    boundsMap.FindSegmentBlockIntersections(this->SelfBlockId, origin, samplePoint, tMin, tMax);

    vtkm::Id hitOffset = offset;
    for (vtkm::Id block = 0; block < this->NumBlocks; block++)
    {
      if (block == this->SelfBlockId)
        continue;

      bool hitsBlock = boundsMap.FindSegmentBlockIntersections(
        block, origin, samplePoint, tMin, tMax, this->SelfBlockId);

      if (!hitsBlock)
        continue;

      if ((!this->UseGlancingHits) && beams::Intersections::ApproxEquals(tMin, tMax))
        continue;

      TransmittanceRayBlockHit hit;
      hit.RayId = rayId;
      hit.BlockId = block;
      hit.FromBlockId = this->SelfBlockId;
      hit.RayT = tMax;
      hit.Point = origin + dir * tMax;
      hits.Set(hitOffset, hit);
      hitOffset++;
    }
  }

  vtkm::Id SelfBlockId;
  vtkm::Id NumBlocks;
  vtkm::Vec3f LightLoc;
  bool UseGlancingHits;
};


template <typename ShadowMapEstimatorType>
struct TransmittanceFetcher : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn minLocations,
                                FieldIn maxLocations,
                                FieldOut transmittances);
  using ExecutionSignature = void(_1, _2, _3);

  VTKM_CONT
  TransmittanceFetcher(vtkm::Float32 stepSize, const ShadowMapEstimatorType& shadowMapEstimator)
    : StepSize(stepSize)
    , ShadowMapEstimator(shadowMapEstimator)
  {
  }

  VTKM_EXEC void operator()(const vtkm::Vec4f_32& minLocation,
                            const vtkm::Vec4f_32& maxLocation,
                            vtkm::Float32& opacity) const
  {
    vtkm::Vec3f_32 start{ minLocation[0], minLocation[1], minLocation[2] };
    vtkm::Vec3f_32 end{ maxLocation[0], maxLocation[1], maxLocation[2] };

    opacity = this->ShadowMapEstimator.GetEstimateUsingVerticesT(end) * (1.0f - opacity);
  }

  vtkm::Float32 StepSize;
  ShadowMapEstimatorType ShadowMapEstimator;
  bool GlancingHits;
};

template <typename ShadowMapEstimatorType>
struct TransmittanceFetcher2 : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldInOut pullHits);
  using ExecutionSignature = void(_1);

  VTKM_CONT
  TransmittanceFetcher2(int selfBlockId,
                        vtkm::Float32 stepSize,
                        const ShadowMapEstimatorType& shadowMapEstimator)

    : SelfBlockId(selfBlockId)
    , StepSize(stepSize)
    , ShadowMapEstimator(shadowMapEstimator)
  {
  }

  VTKM_EXEC void operator()(TransmittanceRayBlockHit& pullHit) const
  {
    pullHit.Opacity = this->ShadowMapEstimator.GetEstimateUsingVerticesT(pullHit.Point);
  }

  int SelfBlockId;
  vtkm::Float32 StepSize;
  ShadowMapEstimatorType ShadowMapEstimator;
};

vtkm::Float32 GetMaxAlpha(const vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 4>>& colorMap)
{
  vtkm::Float32 maxAlpha = 0.f;
  vtkm::Id size = colorMap.GetNumberOfValues();
  auto portal = colorMap.ReadPortal();
  for (vtkm::Id i = 0; i < size; ++i)
  {
    maxAlpha = vtkm::Max(maxAlpha, portal.Get(i)[3]);
  }
  return maxAlpha;
}

void SaveTransmittance(const vtkm::cont::DataSet& ds, const std::string& suffix)
{
  auto mpi = beams::mpi::MpiEnv::Get();
  std::stringstream ss;
  std::string s = (suffix.size() > 0) ? ("_" + suffix) : "";
  ss << "transmittances"
     << "_" << mpi->Rank << s << ".vtk";
  Fmt::Println("Saving {}", ss.str());
  vtkm::io::VTKDataSetWriter writer(ss.str());
  writer.WriteDataSet(ds);
}

template <typename T, vtkm::IdComponent S>
VTKM_EXEC_CONT vtkm::Vec<vtkm::Float32, S> ToVecf32(const vtkm::Vec<T, S>& v)
{
  vtkm::Vec<vtkm::Float32, S> r;
  for (vtkm::IdComponent i = 0; i < S; ++i)
  {
    r[i] = static_cast<vtkm::Float32>(v[i]);
  }
  return r;
}

vtkm::cont::DataSet CreateDataSetForOpacityMap(const vtkm::Vec3f_32& origin,
                                               const vtkm::Vec3f_32& size,
                                               const vtkm::Id3& dims)
{
  vtkm::Id3 pdims{ dims + vtkm::Id3{ 1, 1, 1 } };
  vtkm::Vec3f_32 spacing = size / dims;
  vtkm::cont::ArrayHandleUniformPointCoordinates coordinates(pdims, origin, spacing);
  vtkm::cont::CellSetStructured<3> cellSet;
  cellSet.SetPointDimensions(pdims);
  vtkm::cont::DataSet dataSet;
  dataSet.SetCellSet(cellSet);
  dataSet.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", coordinates));
  return dataSet;
}

template <typename Device, typename OracleType, typename Precision>
beams::rendering::TransmittanceMapEstimator<Device, beams::rendering::TransmittanceLocator<Device>>
GenerateEstimator(const vtkm::Bounds& bounds,
                  const vtkm::Id3& dims,
                  vtkm::cont::DataSet& dataSet,
                  beams::rendering::LightRays<Precision, Device>& lightRays,
                  const vtkm::Range& scalarRange,
                  const vtkm::cont::Field* scalarField,
                  vtkm::rendering::raytracing::Lights& lights,
                  OracleType& oracle,
                  const vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 4>>& correctedColorMap,
                  vtkm::cont::ArrayHandle<vtkm::Float32>& opacities,
                  vtkm::cont::Token& token)
{
  using CoordinatesArrayHandle = vtkm::cont::ArrayHandleUniformPointCoordinates;
  // Generate uniform dataset within same bounds as the data set to render
  // but with the size of dims
  vtkm::Vec3f_32 size = ToVecf32(vtkm::Vec3f_64{
    bounds.X.Max - bounds.X.Min, bounds.Y.Max - bounds.Y.Min, bounds.Z.Max - bounds.Z.Min });
  vtkm::Float32 numSteps = 128.0f;
  vtkm::Float32 stepSize = vtkm::Magnitude(size) / numSteps;
  // FMT_VAR(size);
  // FMT_VAR(stepSize);
  vtkm::Id3 pdims{ dims + vtkm::Id3{ 1, 1, 1 } };

  // Find the centers of the cells of the created uniform dataset
  auto lightColor = lights.Colors[0];
  auto lightLoc = lights.Locations[0];

  auto coordinates =
    dataSet.GetCoordinateSystem().GetData().AsArrayHandle<CoordinatesArrayHandle>();
  const vtkm::Float32 maxDensity = GetMaxAlpha(correctedColorMap);
  vtkm::cont::Invoker photonMapGenInvoker{ Device() };
  photonMapGenInvoker(TransmittanceMapGenerator{ stepSize,
                                                 vtkm::Float32(scalarRange.Min),
                                                 vtkm::Float32(scalarRange.Max),
                                                 maxDensity,
                                                 lightLoc,
                                                 bounds },
                      lightRays.Ids,
                      lightRays.Origins,
                      lightRays.Dirs,
                      lightRays.Dests,
                      lights,
                      oracle,
                      vtkm::rendering::raytracing::GetScalarFieldArray(*scalarField),
                      correctedColorMap,
                      opacities);

  dataSet.AddPointField("transmittance", opacities);

  TransmittanceLocator<Device> locator(coordinates, pdims, token);
  TransmittanceMapEstimator<Device, TransmittanceLocator<Device>> transmittanceMapEstimator(
    coordinates, opacities, locator, lightColor, token);

  return transmittanceMapEstimator;
}

template <typename TransmittanceEstimator, typename Device>
void GetNonLocalHits(TransmittanceEstimator& transmittanceEstimator,
                     const vtkm::rendering::raytracing::Lights& lights,
                     const beams::rendering::BoundsMap& boundsMap,
                     bool useGlancingHits,
                     vtkm::cont::ArrayHandle<vtkm::Id>& hitCounts,
                     vtkm::cont::ArrayHandle<vtkm::Id>& hitOffsets,
                     vtkm::cont::ArrayHandle<TransmittanceRayBlockHit>& hits)
{
  auto mpi = beams::mpi::MpiEnv::Get();
  vtkm::cont::Invoker invoker{ Device() };

  invoker(CountNonLocalBlockHits{ mpi->Rank, lights.Locations[0], useGlancingHits },
          transmittanceEstimator.LocationsHandle,
          boundsMap,
          hitCounts);
  vtkm::Id totalHitCount = vtkm::cont::Algorithm::Reduce(hitCounts, 0);
  vtkm::cont::Algorithm::ScanExclusive(hitCounts, hitOffsets);
  // Fmt::PrintArrayHandlelnr(1, "hitOffsets", hitOffsets);

  hits.Allocate(totalHitCount);
  invoker(CalculateNonLocalBlockHits{ mpi->Rank, mpi->Size, lights.Locations[0], useGlancingHits },
          transmittanceEstimator.LocationsHandle,
          boundsMap,
          hitOffsets,
          hits);
}

} // namespace rendering
} // namespace beams

#endif // beams_rendering_transmittance_map_h
