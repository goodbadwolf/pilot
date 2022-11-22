#include "LightedVolumeRenderer.h"
#include "../Math.h"
#include "PointLight.h"
#include "TransmittanceMap.h"
#include <pilot/Logger.h>

#include "RectilinearMeshOracle.h"
#include <vtkm/cont/ArrayHandleCartesianProduct.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/ColorTable.h>
#include <vtkm/cont/ErrorBadValue.h>
#include <vtkm/cont/Timer.h>
#include <vtkm/cont/TryExecute.h>
#include <vtkm/io/VTKDataSetWriter.h>
#include <vtkm/rendering/raytracing/Camera.h>
#include <vtkm/rendering/raytracing/Logger.h>
#include <vtkm/rendering/raytracing/Ray.h>
#include <vtkm/rendering/raytracing/RayTracingTypeDefs.h>
#include <vtkm/thirdparty/diy/diy.h>
#include <vtkm/thirdparty/diy/mpi-cast.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <mpi.h>

#include <math.h>
#include <numeric>
#include <stdio.h>

using namespace vtkm::rendering::raytracing;

extern vtkm::Float64 Phase1Time;
extern vtkm::Float64 Phase2Time;
extern vtkm::Float64 Phase3Time;
extern vtkm::Float64 Phase4Time;

namespace beams
{
namespace rendering
{
namespace
{
template <typename Device>
class RectilinearLocator
{
protected:
  using DefaultHandle = vtkm::cont::ArrayHandle<vtkm::FloatDefault>;
  using CartesianArrayHandle =
    vtkm::cont::ArrayHandleCartesianProduct<DefaultHandle, DefaultHandle, DefaultHandle>;
  using DefaultConstHandle = typename DefaultHandle::ReadPortalType;
  using CartesianConstPortal = typename CartesianArrayHandle::ReadPortalType;

  DefaultConstHandle CoordPortals[3];
  CartesianConstPortal Coordinates;
  vtkm::exec::ConnectivityStructured<vtkm::TopologyElementTagCell, vtkm::TopologyElementTagPoint, 3>
    Conn;
  vtkm::Id3 PointDimensions;
  vtkm::Vec3f_32 MinPoint;
  vtkm::Vec3f_32 MaxPoint;

public:
  RectilinearLocator(const CartesianArrayHandle& coordinates,
                     vtkm::cont::CellSetStructured<3>& cellset,
                     vtkm::cont::Token& token)
    : Coordinates(coordinates.PrepareForInput(Device(), token))
    , Conn(cellset.PrepareForInput(Device(),
                                   vtkm::TopologyElementTagCell(),
                                   vtkm::TopologyElementTagPoint(),
                                   token))
  {
    CoordPortals[0] = Coordinates.GetFirstPortal();
    CoordPortals[1] = Coordinates.GetSecondPortal();
    CoordPortals[2] = Coordinates.GetThirdPortal();
    PointDimensions = Conn.GetPointDimensions();
    MinPoint[0] = static_cast<vtkm::Float32>(coordinates.ReadPortal().GetFirstPortal().Get(0));
    MinPoint[1] = static_cast<vtkm::Float32>(coordinates.ReadPortal().GetSecondPortal().Get(0));
    MinPoint[2] = static_cast<vtkm::Float32>(coordinates.ReadPortal().GetThirdPortal().Get(0));

    MaxPoint[0] = static_cast<vtkm::Float32>(
      coordinates.ReadPortal().GetFirstPortal().Get(PointDimensions[0] - 1));
    MaxPoint[1] = static_cast<vtkm::Float32>(
      coordinates.ReadPortal().GetSecondPortal().Get(PointDimensions[1] - 1));
    MaxPoint[2] = static_cast<vtkm::Float32>(
      coordinates.ReadPortal().GetThirdPortal().Get(PointDimensions[2] - 1));
  }

  VTKM_EXEC
  inline bool IsInside(const vtkm::Vec3f_32& point) const
  {
    bool inside = true;
    if (point[0] < MinPoint[0] || point[0] > MaxPoint[0])
      inside = false;
    if (point[1] < MinPoint[1] || point[1] > MaxPoint[1])
      inside = false;
    if (point[2] < MinPoint[2] || point[2] > MaxPoint[2])
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

  //
  // Assumes point inside the data set
  //
  VTKM_EXEC
  inline void LocateCell(vtkm::Id3& cell,
                         const vtkm::Vec3f_32& point,
                         vtkm::Vec3f_32& invSpacing) const
  {
    for (vtkm::Int32 dim = 0; dim < 3; ++dim)
    {
      //
      // When searching for points, we consider the max value of the cell
      // to be apart of the next cell. If the point falls on the boundary of the
      // data set, then it is technically inside a cell. This checks for that case
      //
      if (point[dim] == MaxPoint[dim])
      {
        cell[dim] = PointDimensions[dim] - 2;
        continue;
      }

      bool found = false;
      vtkm::Float32 minVal = static_cast<vtkm::Float32>(CoordPortals[dim].Get(cell[dim]));
      const vtkm::Id searchDir = (point[dim] - minVal >= 0.f) ? 1 : -1;
      vtkm::Float32 maxVal = static_cast<vtkm::Float32>(CoordPortals[dim].Get(cell[dim] + 1));

      while (!found)
      {
        if (point[dim] >= minVal && point[dim] < maxVal)
        {
          found = true;
          continue;
        }

        cell[dim] += searchDir;
        vtkm::Id nextCellId = searchDir == 1 ? cell[dim] + 1 : cell[dim];
        vtkm::Float32 next = static_cast<vtkm::Float32>(CoordPortals[dim].Get(nextCellId));
        if (searchDir == 1)
        {
          minVal = maxVal;
          maxVal = next;
        }
        else
        {
          maxVal = minVal;
          minVal = next;
        }
      }
      invSpacing[dim] = 1.f / (maxVal - minVal);
    }
  } // LocateCell

  VTKM_EXEC
  inline vtkm::Id GetCellIndex(const vtkm::Id3& cell) const
  {
    return (cell[2] * (PointDimensions[1] - 1) + cell[1]) * (PointDimensions[0] - 1) + cell[0];
  }

  VTKM_EXEC
  inline void GetPoint(const vtkm::Id& index, vtkm::Vec3f_32& point) const
  {
    point = Coordinates.Get(index);
  }

  VTKM_EXEC
  inline void GetMinPoint(const vtkm::Id3& cell, vtkm::Vec3f_32& point) const
  {
    const vtkm::Id pointIndex =
      (cell[2] * PointDimensions[1] + cell[1]) * PointDimensions[0] + cell[0];
    point = Coordinates.Get(pointIndex);
  }
}; // class RectilinearLocator

template <typename Device>
class UniformLocator
{
protected:
  using PointsArrayHandle = vtkm::cont::ArrayHandleUniformPointCoordinates;
  using PointsReadPortal = typename PointsArrayHandle::ReadPortalType;

  vtkm::Id3 PointDimensions;
  vtkm::Vec3f_32 Origin;
  vtkm::Vec3f_32 InvSpacing;
  vtkm::Vec3f_32 MaxPoint;
  PointsReadPortal Coordinates;
  vtkm::exec::ConnectivityStructured<vtkm::TopologyElementTagCell, vtkm::TopologyElementTagPoint, 3>
    Conn;

public:
  UniformLocator(const PointsArrayHandle& coordinates,
                 vtkm::cont::CellSetStructured<3>& cellset,
                 vtkm::cont::Token& token)
    : Coordinates(coordinates.PrepareForInput(Device(), token))
    , Conn(cellset.PrepareForInput(Device(),
                                   vtkm::TopologyElementTagCell(),
                                   vtkm::TopologyElementTagPoint(),
                                   token))
  {
    Origin = Coordinates.GetOrigin();
    PointDimensions = Conn.GetPointDimensions();
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
  inline void LocateCell(vtkm::Id3& cell,
                         const vtkm::Vec3f_32& point,
                         vtkm::Vec3f_32& invSpacing) const
  {
    vtkm::Vec3f_32 temp = point;
    temp = temp - Origin;
    temp = temp * InvSpacing;
    //make sure that if we border the upper edge, we sample the correct cell
    if (temp[0] == vtkm::Float32(PointDimensions[0] - 1))
      temp[0] = vtkm::Float32(PointDimensions[0] - 2);
    if (temp[1] == vtkm::Float32(PointDimensions[1] - 1))
      temp[1] = vtkm::Float32(PointDimensions[1] - 2);
    if (temp[2] == vtkm::Float32(PointDimensions[2] - 1))
      temp[2] = vtkm::Float32(PointDimensions[2] - 2);
    cell = temp;
    invSpacing = InvSpacing;
  }

  VTKM_EXEC
  inline void GetPoint(const vtkm::Id& index, vtkm::Vec3f_32& point) const
  {
    point = Coordinates.Get(index);
  }

  VTKM_EXEC
  inline void GetMinPoint(const vtkm::Id3& cell, vtkm::Vec3f_32& point) const
  {
    const vtkm::Id pointIndex =
      (cell[2] * PointDimensions[1] + cell[1]) * PointDimensions[0] + cell[0];
    point = Coordinates.Get(pointIndex);
  }

}; // class UniformLocator

template <typename DeviceAdapterTag, typename LocatorType, typename MapEstimatorType>
class Sampler : public vtkm::worklet::WorkletMapField
{
private:
  using ColorArrayHandle = typename vtkm::cont::ArrayHandle<vtkm::Vec4f_32>;
  using ColorArrayPortal = typename ColorArrayHandle::ReadPortalType;
  ColorArrayPortal ColorMap;
  vtkm::Id ColorMapSize;
  vtkm::Float32 MinScalar;
  vtkm::Float32 SampleDistance;
  vtkm::Float32 InverseDeltaScalar;
  LocatorType Locator;
  vtkm::Float32 MeshEpsilon;
  MapEstimatorType MapEstimator;
  bool UseMap;

public:
  VTKM_CONT
  Sampler(const ColorArrayHandle& colorMap,
          const vtkm::Float32& minScalar,
          const vtkm::Float32& maxScalar,
          const vtkm::Float32& sampleDistance,
          const LocatorType& locator,
          const vtkm::Float32& meshEpsilon,
          const MapEstimatorType& shadowMapEstimator,
          bool useMap,
          vtkm::cont::Token& token)
    : ColorMap(colorMap.PrepareForInput(DeviceAdapterTag(), token))
    , MinScalar(minScalar)
    , SampleDistance(sampleDistance)
    , InverseDeltaScalar(minScalar)
    , Locator(locator)
    , MeshEpsilon(meshEpsilon)
    , MapEstimator(shadowMapEstimator)
    , UseMap(useMap)
  {
    ColorMapSize = colorMap.GetNumberOfValues() - 1;
    if ((maxScalar - minScalar) != 0.f)
    {
      InverseDeltaScalar = 1.f / (maxScalar - minScalar);
    }
  }

  using ControlSignature = void(FieldIn, FieldIn, FieldIn, FieldIn, WholeArrayInOut, WholeArrayIn);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, WorkIndex);

  template <typename Precision>
  VTKM_EXEC_CONT static bool ApproxEquals(Precision x, Precision y, Precision eps = 1e-5f)
  {
    return vtkm::Abs(x - y) <= eps;
  }
  template <typename ScalarPortalType, typename ColorBufferType>
  VTKM_EXEC void operator()(const vtkm::Vec3f_32& rayDir,
                            const vtkm::Vec3f_32& rayOrigin,
                            const vtkm::Float32& minDistance,
                            const vtkm::Float32& maxDistance,
                            ColorBufferType& colorBuffer,
                            ScalarPortalType& scalars,
                            const vtkm::Id& pixelIndex) const
  {
    vtkm::Vec4f_32 color;
    color[0] = colorBuffer.Get(pixelIndex * 4 + 0);
    color[1] = colorBuffer.Get(pixelIndex * 4 + 1);
    color[2] = colorBuffer.Get(pixelIndex * 4 + 2);
    color[3] = colorBuffer.Get(pixelIndex * 4 + 3);

    if (minDistance == -1.f)
    {
      return; //TODO: Compact? or just image subset...
    }
    //get the initial sample position;
    vtkm::Vec3f_32 sampleLocation;
    // find the distance to the first sample
    vtkm::Float32 distance = minDistance + MeshEpsilon;
    sampleLocation = rayOrigin + distance * rayDir;
    // since the calculations are slightly different, we could hit an
    // edge case where the first sample location may not be in the data set.
    // Thus, advance to the next sample location
    while (!Locator.IsInside(sampleLocation) && distance < maxDistance)
    {
      distance += SampleDistance;
      sampleLocation = rayOrigin + distance * rayDir;
    }
    /*
            7----------6
           /|         /|
          4----------5 |
          | |        | |
          | 3--------|-2    z y
          |/         |/     |/
          0----------1      |__ x
    */
    vtkm::Vec3f_32 bottomLeft(0, 0, 0);
    bool newCell = true;
    //check to see if we left the cell
    vtkm::Float32 tx = 0.f;
    vtkm::Float32 ty = 0.f;
    vtkm::Float32 tz = 0.f;
    vtkm::Float32 scalar0 = 0.f;
    vtkm::Float32 scalar1minus0 = 0.f;
    vtkm::Float32 scalar2minus3 = 0.f;
    vtkm::Float32 scalar3 = 0.f;
    vtkm::Float32 scalar4 = 0.f;
    vtkm::Float32 scalar5minus4 = 0.f;
    vtkm::Float32 scalar6minus7 = 0.f;
    vtkm::Float32 scalar7 = 0.f;

    vtkm::Id3 cell(0, 0, 0);
    vtkm::Vec3f_32 invSpacing(0.f, 0.f, 0.f);

    while (Locator.IsInside(sampleLocation) && distance < maxDistance)
    {
      vtkm::Float32 mint = vtkm::Min(tx, vtkm::Min(ty, tz));
      vtkm::Float32 maxt = vtkm::Max(tx, vtkm::Max(ty, tz));
      if (maxt > 1.f || mint < 0.f)
        newCell = true;

      if (newCell)
      {

        vtkm::Vec<vtkm::Id, 8> cellIndices;
        Locator.LocateCell(cell, sampleLocation, invSpacing);
        Locator.GetCellIndices(cell, cellIndices);
        Locator.GetPoint(cellIndices[0], bottomLeft);

        scalar0 = vtkm::Float32(scalars.Get(cellIndices[0]));
        vtkm::Float32 scalar1 = vtkm::Float32(scalars.Get(cellIndices[1]));
        vtkm::Float32 scalar2 = vtkm::Float32(scalars.Get(cellIndices[2]));
        scalar3 = vtkm::Float32(scalars.Get(cellIndices[3]));
        scalar4 = vtkm::Float32(scalars.Get(cellIndices[4]));
        vtkm::Float32 scalar5 = vtkm::Float32(scalars.Get(cellIndices[5]));
        vtkm::Float32 scalar6 = vtkm::Float32(scalars.Get(cellIndices[6]));
        scalar7 = vtkm::Float32(scalars.Get(cellIndices[7]));

        // save ourselves a couple extra instructions
        scalar6minus7 = scalar6 - scalar7;
        scalar5minus4 = scalar5 - scalar4;
        scalar1minus0 = scalar1 - scalar0;
        scalar2minus3 = scalar2 - scalar3;

        tx = (sampleLocation[0] - bottomLeft[0]) * invSpacing[0];
        ty = (sampleLocation[1] - bottomLeft[1]) * invSpacing[1];
        tz = (sampleLocation[2] - bottomLeft[2]) * invSpacing[2];

        newCell = false;
      }

      vtkm::Float32 lerped76 = scalar7 + tx * scalar6minus7;
      vtkm::Float32 lerped45 = scalar4 + tx * scalar5minus4;
      vtkm::Float32 lerpedTop = lerped45 + ty * (lerped76 - lerped45);

      vtkm::Float32 lerped01 = scalar0 + tx * scalar1minus0;
      vtkm::Float32 lerped32 = scalar3 + tx * scalar2minus3;
      vtkm::Float32 lerpedBottom = lerped01 + ty * (lerped32 - lerped01);

      vtkm::Float32 finalScalar = lerpedBottom + tz * (lerpedTop - lerpedBottom);
      //normalize scalar
      finalScalar = (finalScalar - MinScalar) * InverseDeltaScalar;

      vtkm::Id colorIndex =
        static_cast<vtkm::Id>(finalScalar * static_cast<vtkm::Float32>(ColorMapSize));
      vtkm::Vec4f_32 sampleColor;
      if (colorIndex < 0)
        continue;
      else if (colorIndex > ColorMapSize)
        continue;
      else
        sampleColor = ColorMap.Get(colorIndex);

      //composite
      sampleColor[3] *= (1.f - color[3]);

      if (this->UseMap)
      {
        auto directEstimate = this->MapEstimator.GetEstimateUsingVertices(sampleLocation);
        sampleColor[0] = sampleColor[0] * directEstimate[0];
        sampleColor[1] = sampleColor[1] * directEstimate[1];
        sampleColor[2] = sampleColor[2] * directEstimate[2];
      }

      sampleColor[0] = vtkm::Clamp(sampleColor[0], 0.0f, 1.0f);
      sampleColor[1] = vtkm::Clamp(sampleColor[1], 0.0f, 1.0f);
      sampleColor[2] = vtkm::Clamp(sampleColor[2], 0.0f, 1.0f);

      color[0] = color[0] + sampleColor[0] * sampleColor[3];
      color[1] = color[1] + sampleColor[1] * sampleColor[3];
      color[2] = color[2] + sampleColor[2] * sampleColor[3];
      color[3] = sampleColor[3] + color[3];
      //advance
      distance += SampleDistance;
      sampleLocation = sampleLocation + SampleDistance * rayDir;

      //this is linear could just do an addition
      tx = (sampleLocation[0] - bottomLeft[0]) * invSpacing[0];
      ty = (sampleLocation[1] - bottomLeft[1]) * invSpacing[1];
      tz = (sampleLocation[2] - bottomLeft[2]) * invSpacing[2];

      if (color[3] >= 1.f)
        break;
    }
    colorBuffer.Set(pixelIndex * 4 + 0, color[0]);
    colorBuffer.Set(pixelIndex * 4 + 1, color[1]);
    colorBuffer.Set(pixelIndex * 4 + 2, color[2]);
    colorBuffer.Set(pixelIndex * 4 + 3, color[3]);
  }
}; //Sampler

template <typename DeviceAdapterTag, typename LocatorType>
class SamplerCellAssoc : public vtkm::worklet::WorkletMapField
{
private:
  using ColorArrayHandle = typename vtkm::cont::ArrayHandle<vtkm::Vec4f_32>;
  using ColorArrayPortal = typename ColorArrayHandle::ReadPortalType;
  ColorArrayPortal ColorMap;
  vtkm::Id ColorMapSize;
  vtkm::Float32 MinScalar;
  vtkm::Float32 SampleDistance;
  vtkm::Float32 InverseDeltaScalar;
  LocatorType Locator;
  vtkm::Float32 MeshEpsilon;

public:
  VTKM_CONT
  SamplerCellAssoc(const ColorArrayHandle& colorMap,
                   const vtkm::Float32& minScalar,
                   const vtkm::Float32& maxScalar,
                   const vtkm::Float32& sampleDistance,
                   const LocatorType& locator,
                   const vtkm::Float32& meshEpsilon,
                   vtkm::cont::Token& token)
    : ColorMap(colorMap.PrepareForInput(DeviceAdapterTag(), token))
    , MinScalar(minScalar)
    , SampleDistance(sampleDistance)
    , InverseDeltaScalar(minScalar)
    , Locator(locator)
    , MeshEpsilon(meshEpsilon)
  {
    ColorMapSize = colorMap.GetNumberOfValues() - 1;
    if ((maxScalar - minScalar) != 0.f)
    {
      InverseDeltaScalar = 1.f / (maxScalar - minScalar);
    }
  }
  using ControlSignature = void(FieldIn, FieldIn, FieldIn, FieldIn, WholeArrayInOut, WholeArrayIn);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, WorkIndex);

  template <typename ScalarPortalType, typename ColorBufferType>
  VTKM_EXEC void operator()(const vtkm::Vec3f_32& rayDir,
                            const vtkm::Vec3f_32& rayOrigin,
                            const vtkm::Float32& minDistance,
                            const vtkm::Float32& maxDistance,
                            ColorBufferType& colorBuffer,
                            const ScalarPortalType& scalars,
                            const vtkm::Id& pixelIndex) const
  {
    vtkm::Vec4f_32 color;
    color[0] = colorBuffer.Get(pixelIndex * 4 + 0);
    color[1] = colorBuffer.Get(pixelIndex * 4 + 1);
    color[2] = colorBuffer.Get(pixelIndex * 4 + 2);
    color[3] = colorBuffer.Get(pixelIndex * 4 + 3);

    if (minDistance == -1.f)
      return; //TODO: Compact? or just image subset...
    //get the initial sample position;
    vtkm::Vec3f_32 sampleLocation;
    // find the distance to the first sample
    vtkm::Float32 distance = minDistance + MeshEpsilon;
    sampleLocation = rayOrigin + distance * rayDir;
    // since the calculations are slightly different, we could hit an
    // edge case where the first sample location may not be in the data set.
    // Thus, advance to the next sample location
    while (!Locator.IsInside(sampleLocation) && distance < maxDistance)
    {
      distance += SampleDistance;
      sampleLocation = rayOrigin + distance * rayDir;
    }

    /*
            7----------6
           /|         /|
          4----------5 |
          | |        | |
          | 3--------|-2    z y
          |/         |/     |/
          0----------1      |__ x
    */
    bool newCell = true;
    vtkm::Float32 tx = 2.f;
    vtkm::Float32 ty = 2.f;
    vtkm::Float32 tz = 2.f;
    vtkm::Float32 scalar0 = 0.f;
    vtkm::Vec4f_32 sampleColor(0.f, 0.f, 0.f, 0.f);
    vtkm::Vec3f_32 bottomLeft(0.f, 0.f, 0.f);
    vtkm::Vec3f_32 invSpacing(0.f, 0.f, 0.f);
    vtkm::Id3 cell(0, 0, 0);
    while (Locator.IsInside(sampleLocation) && distance < maxDistance)
    {
      vtkm::Float32 mint = vtkm::Min(tx, vtkm::Min(ty, tz));
      vtkm::Float32 maxt = vtkm::Max(tx, vtkm::Max(ty, tz));
      if (maxt > 1.f || mint < 0.f)
        newCell = true;
      if (newCell)
      {
        Locator.LocateCell(cell, sampleLocation, invSpacing);
        vtkm::Id cellId = Locator.GetCellIndex(cell);

        scalar0 = vtkm::Float32(scalars.Get(cellId));
        vtkm::Float32 normalizedScalar = (scalar0 - MinScalar) * InverseDeltaScalar;
        vtkm::Id colorIndex =
          static_cast<vtkm::Id>(normalizedScalar * static_cast<vtkm::Float32>(ColorMapSize));
        if (colorIndex < 0)
          colorIndex = 0;
        if (colorIndex > ColorMapSize)
          colorIndex = ColorMapSize;
        sampleColor = ColorMap.Get(colorIndex);
        Locator.GetMinPoint(cell, bottomLeft);
        tx = (sampleLocation[0] - bottomLeft[0]) * invSpacing[0];
        ty = (sampleLocation[1] - bottomLeft[1]) * invSpacing[1];
        tz = (sampleLocation[2] - bottomLeft[2]) * invSpacing[2];
        newCell = false;
      }

      // just repeatably composite
      vtkm::Float32 alpha = sampleColor[3] * (1.f - color[3]);
      color[0] = color[0] + sampleColor[0] * alpha;
      color[1] = color[1] + sampleColor[1] * alpha;
      color[2] = color[2] + sampleColor[2] * alpha;
      color[3] = alpha + color[3];
      //advance
      distance += SampleDistance;
      sampleLocation = sampleLocation + SampleDistance * rayDir;

      if (color[3] >= 1.f)
        break;
      tx = (sampleLocation[0] - bottomLeft[0]) * invSpacing[0];
      ty = (sampleLocation[1] - bottomLeft[1]) * invSpacing[1];
      tz = (sampleLocation[2] - bottomLeft[2]) * invSpacing[2];
    }
    color[0] = vtkm::Min(color[0], 1.f);
    color[1] = vtkm::Min(color[1], 1.f);
    color[2] = vtkm::Min(color[2], 1.f);
    color[3] = vtkm::Min(color[3], 1.f);

    colorBuffer.Set(pixelIndex * 4 + 0, color[0]);
    colorBuffer.Set(pixelIndex * 4 + 1, color[1]);
    colorBuffer.Set(pixelIndex * 4 + 2, color[2]);
    colorBuffer.Set(pixelIndex * 4 + 3, color[3]);
  }
}; //SamplerCell

class CalcRayStart : public vtkm::worklet::WorkletMapField
{
  vtkm::Float32 Xmin;
  vtkm::Float32 Ymin;
  vtkm::Float32 Zmin;
  vtkm::Float32 Xmax;
  vtkm::Float32 Ymax;
  vtkm::Float32 Zmax;

public:
  VTKM_CONT
  CalcRayStart(const vtkm::Bounds boundingBox)
  {
    Xmin = static_cast<vtkm::Float32>(boundingBox.X.Min);
    Xmax = static_cast<vtkm::Float32>(boundingBox.X.Max);
    Ymin = static_cast<vtkm::Float32>(boundingBox.Y.Min);
    Ymax = static_cast<vtkm::Float32>(boundingBox.Y.Max);
    Zmin = static_cast<vtkm::Float32>(boundingBox.Z.Min);
    Zmax = static_cast<vtkm::Float32>(boundingBox.Z.Max);
  }

  VTKM_EXEC
  vtkm::Float32 rcp(vtkm::Float32 f) const { return 1.0f / f; }

  VTKM_EXEC
  vtkm::Float32 rcp_safe(vtkm::Float32 f) const { return rcp((fabs(f) < 1e-8f) ? 1e-8f : f); }

  using ControlSignature = void(FieldIn, FieldOut, FieldInOut, FieldInOut, FieldIn);
  using ExecutionSignature = void(_1, _2, _3, _4, _5);
  template <typename Precision>
  VTKM_EXEC void operator()(const vtkm::Vec<Precision, 3>& rayDir,
                            vtkm::Float32& minDistance,
                            vtkm::Float32& distance,
                            vtkm::Float32& maxDistance,
                            const vtkm::Vec<Precision, 3>& rayOrigin) const
  {
    vtkm::Float32 dirx = static_cast<vtkm::Float32>(rayDir[0]);
    vtkm::Float32 diry = static_cast<vtkm::Float32>(rayDir[1]);
    vtkm::Float32 dirz = static_cast<vtkm::Float32>(rayDir[2]);
    vtkm::Float32 origx = static_cast<vtkm::Float32>(rayOrigin[0]);
    vtkm::Float32 origy = static_cast<vtkm::Float32>(rayOrigin[1]);
    vtkm::Float32 origz = static_cast<vtkm::Float32>(rayOrigin[2]);

    vtkm::Float32 invDirx = rcp_safe(dirx);
    vtkm::Float32 invDiry = rcp_safe(diry);
    vtkm::Float32 invDirz = rcp_safe(dirz);

    vtkm::Float32 odirx = origx * invDirx;
    vtkm::Float32 odiry = origy * invDiry;
    vtkm::Float32 odirz = origz * invDirz;

    vtkm::Float32 xmin = Xmin * invDirx - odirx;
    vtkm::Float32 ymin = Ymin * invDiry - odiry;
    vtkm::Float32 zmin = Zmin * invDirz - odirz;
    vtkm::Float32 xmax = Xmax * invDirx - odirx;
    vtkm::Float32 ymax = Ymax * invDiry - odiry;
    vtkm::Float32 zmax = Zmax * invDirz - odirz;


    minDistance = vtkm::Max(
      vtkm::Max(vtkm::Max(vtkm::Min(ymin, ymax), vtkm::Min(xmin, xmax)), vtkm::Min(zmin, zmax)),
      minDistance);
    vtkm::Float32 exitDistance =
      vtkm::Min(vtkm::Min(vtkm::Max(ymin, ymax), vtkm::Max(xmin, xmax)), vtkm::Max(zmin, zmax));
    maxDistance = vtkm::Min(maxDistance, exitDistance);
    if (maxDistance < minDistance)
    {
      minDistance = -1.f; //flag for miss
    }
    else
    {
      distance = minDistance;
    }
  }
}; //class CalcRayStart

struct TransmissionDataRequest
{
  std::vector<int> FromRanks;
  int ToRank;

  std::vector<int> BlockIds;
  std::vector<vtkm::Vec3f_32> HitMins;
  std::vector<vtkm::Vec3f_32> HitMaxs;

  // Number of BlockIds, flat, needed for MPI
  // int NumBlockIds;
  // Number of Hits, flat, needed for MPI
  int NumHits;
};

struct TransmissionDataPull
{
  int NumHits = 0;
  std::vector<vtkm::Vec4f_32> HitMins;
  std::vector<vtkm::Vec4f_32> HitMaxs;
  std::vector<vtkm::Float32> Transmittances;
};

struct MpiTypes
{
  MPI_Datatype Vec3f_32;
  MPI_Datatype Vec4f_32;
  MPI_Datatype TransmittanceRayBlockHit;
};

} //namespace

LightedVolumeRenderer::LightedVolumeRenderer()
{
  IsSceneDirty = false;
  IsUniformDataSet = true;
  SampleDistance = -1.f;
  UseShadowMap = true;
  ShadowMapSize = { 16, 16, 16 };
}

void LightedVolumeRenderer::SetColorMap(const vtkm::cont::ArrayHandle<vtkm::Vec4f_32>& colorMap)
{
  ColorMap = colorMap;
}

void LightedVolumeRenderer::SetData(const vtkm::cont::CoordinateSystem& coords,
                                    const vtkm::cont::Field& scalarField,
                                    const vtkm::cont::CellSetStructured<3>& cellset,
                                    const vtkm::Range& scalarRange)
{
  IsUniformDataSet = !coords.GetData().IsType<CartesianArrayHandle>();
  IsSceneDirty = true;
  SpatialExtent = coords.GetBounds();
  SpatialExtentMagnitude = beams::Math::BoundsMagnitude<vtkm::Float32>(this->SpatialExtent);
  CoordinateSystem = coords;
  ScalarField = &scalarField;
  CellSet = cellset;
  ScalarRange = scalarRange;
}

template <typename Precision>
struct LightedVolumeRenderer::RenderFunctor
{
protected:
  beams::rendering::LightedVolumeRenderer* Self;
  vtkm::rendering::raytracing::Ray<Precision>& Rays;

public:
  VTKM_CONT
  RenderFunctor(beams::rendering::LightedVolumeRenderer* self,
                vtkm::rendering::raytracing::Ray<Precision>& rays)
    : Self(self)
    , Rays(rays)
  {
  }

  template <typename Device>
  VTKM_CONT bool operator()(Device)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);

    this->Self->RenderOnDevice(this->Rays, Device());
    return true;
  }
};

void LightedVolumeRenderer::Render(vtkm::rendering::raytracing::Ray<vtkm::Float32>& rays)
{
  const bool isSupportedField = ScalarField->IsCellField() || ScalarField->IsPointField();
  if (!isSupportedField)
  {
    throw vtkm::cont::ErrorBadValue("Field not accociated with cell set or points");
  }

  if (this->SampleDistance <= 0.f)
  {
    const vtkm::Float32 defaultNumberOfSamples = 200.f;
    this->SampleDistance = this->SpatialExtentMagnitude / defaultNumberOfSamples;
  }

  std::cerr << "SampleDistance = " << this->SampleDistance << "\n";

  RenderFunctor<vtkm::Float32> functor(this, rays);
  vtkm::cont::TryExecute(functor);
}

template <typename Precision, typename Device, typename RectilinearOracleType>
RectilinearOracleType LightedVolumeRenderer::BuildRectilinearOracle()
{
  CartesianArrayHandle cartesianCoords =
    this->CoordinateSystem.GetData().AsArrayHandle<CartesianArrayHandle>();
  return RectilinearOracleType(this->CellSet, cartesianCoords);
}

void LightedVolumeRenderer::AddLight(std::shared_ptr<Light> light)
{
  using PLight = beams::rendering::PointLight<vtkm::Float32>;
  this->Lights.AddLight(light);
  PLight* pLight = reinterpret_cast<PLight*>(light.get());
  TheLights.AddLight(1.0f, pLight->Position, pLight->Color * pLight->Intensity);
}

void LightedVolumeRenderer::ClearLights()
{
  TheLights.ClearLights();
}

template <typename Portal, typename T>
void CopyIntoStdVec(const Portal& portal, std::vector<T>& vec)
{
  vec.resize(portal.GetNumberOfValues());
  for (auto i = 0; i < portal.GetNumberOfValues(); i++)
  {
    vec[i] = portal.Get(i);
  }
}

template <typename PortalType>
VTKM_CONT void CopyPortalToVector(const PortalType& input,
                                  std::vector<typename PortalType::ValueType>& output)
{
  output.resize(static_cast<std ::size_t>(input.GetNumberOfValues()));
  vtkm::cont::ArrayPortalToIterators<PortalType> iterators(input);
  std::copy(iterators.GetBegin(), iterators.GetEnd(), output.begin());
}

template <typename T>
VTKM_CONT void ScanExclusiveVector(const std::vector<T>& input, std::vector<T>& output)
{
  vtkm::cont::ArrayHandle<T> outputAH;
  vtkm::cont::ArrayHandle<T> inputAH = vtkm::cont::make_ArrayHandle(input, vtkm::CopyFlag::Off);
  vtkm::cont::Algorithm::ScanExclusive(inputAH, outputAH);
  CopyPortalToVector(outputAH.ReadPortal(), output);
}

MpiTypes ConstructMpiTypes()
{
  MpiTypes types;
  MPI_Type_contiguous(3, MPI_FLOAT, &types.Vec3f_32);
  MPI_Type_commit(&types.Vec3f_32);
  MPI_Type_contiguous(4, MPI_FLOAT, &types.Vec4f_32);
  MPI_Type_commit(&types.Vec4f_32);

  const int hitMemberCount = 6;
  int hitLengths[hitMemberCount] = { 1, 1, 1, 1, 1, 1 };
  MPI_Aint hitDisplacements[hitMemberCount];
  TransmittanceRayBlockHit dummyHit;
  MPI_Aint hitBaseAddress;
  int addCount = 0;
  MPI_Get_address(&dummyHit, &hitBaseAddress);
  MPI_Get_address(&dummyHit.RayId, &hitDisplacements[addCount++]);
  MPI_Get_address(&dummyHit.BlockId, &hitDisplacements[addCount++]);
  MPI_Get_address(&dummyHit.FromBlockId, &hitDisplacements[addCount++]);
  MPI_Get_address(&dummyHit.Point, &hitDisplacements[addCount++]);
  MPI_Get_address(&dummyHit.RayT, &hitDisplacements[addCount++]);
  MPI_Get_address(&dummyHit.Opacity, &hitDisplacements[addCount++]);
  for (int i = 0; i < hitMemberCount; ++i)
  {
    hitDisplacements[i] = MPI_Aint_diff(hitDisplacements[i], hitBaseAddress);
  }
  MPI_Datatype hitTypes[hitMemberCount] = { MPI_INT,        MPI_INT,   MPI_INT,
                                            types.Vec3f_32, MPI_FLOAT, MPI_FLOAT };
  MPI_Type_create_struct(
    hitMemberCount, hitLengths, hitDisplacements, hitTypes, &types.TransmittanceRayBlockHit);
  MPI_Type_commit(&types.TransmittanceRayBlockHit);

  return types;
}

template <typename Precision, typename Device>
void LightedVolumeRenderer::RenderOnDevice(vtkm::rendering::raytracing::Ray<Precision>& rays,
                                           Device)
{
  this->Profiler->StartFrame("Phase 1");
  LOG::Println0("Phase 1");
  vtkm::cont::Timer phase1ShadowMapTimer{ Device() };
  auto mpi = pilot::mpi::Environment::Get();
  auto comm = mpi->Comm;
  MPI_Comm mpiComm = vtkmdiy::mpi::mpi_cast(comm->handle());

  using OracleType = vtkm::rendering::raytracing::RectilinearMeshOracle;
  OracleType oracle = this->BuildRectilinearOracle<Precision, Device, OracleType>();

  this->Profiler->StartFrame("CreateDataSetForOpacityMap");
  phase1ShadowMapTimer.Start();
  auto bounds = this->SpatialExtent;
  auto dims = this->ShadowMapSize;
  vtkm::Vec3f_32 origin = ToVecf32(vtkm::Vec3f_64{ bounds.X.Min, bounds.Y.Min, bounds.Z.Min });
  vtkm::Vec3f_32 size = ToVecf32(vtkm::Vec3f_64{
    bounds.X.Max - bounds.X.Min, bounds.Y.Max - bounds.Y.Min, bounds.Z.Max - bounds.Z.Min });
  vtkm::cont::DataSet opacityMapDataSet = CreateDataSetForOpacityMap(origin, size, dims);
  this->Profiler->EndFrame();

  using CoordinatesArrayHandle = vtkm::cont::ArrayHandleUniformPointCoordinates;
  auto coordinates =
    opacityMapDataSet.GetCoordinateSystem().GetData().AsArrayHandle<CoordinatesArrayHandle>();

  this->Profiler->StartFrame("CreateRays");
  vtkm::cont::Token token;
  LightRays<vtkm::Float32, Device> lightRays =
    LightRayOperations::CreateRays<vtkm::Float32, CoordinatesArrayHandle, Device>(
      coordinates, TheLights.Locations[0]);
  this->Profiler->EndFrame();

  vtkm::cont::ArrayHandle<vtkm::Float32> opacities;
  vtkm::Id d =
    (this->ShadowMapSize[0] + 1) * (this->ShadowMapSize[1] + 1) * (this->ShadowMapSize[2] + 1);
  opacities.Allocate(d);
  vtkm::cont::Algorithm::Fill(opacities, 0.0f);
  using PhotonMapEstimatorType = TransmittanceMapEstimator<Device, TransmittanceLocator<Device>>;
  PhotonMapEstimatorType transmittanceMapEstimator =
    GenerateEstimator<Device, OracleType, vtkm::Float32>(this->SpatialExtent,
                                                         dims,
                                                         opacityMapDataSet,
                                                         lightRays,
                                                         ScalarRange,
                                                         ScalarField,
                                                         TheLights,
                                                         oracle,
                                                         this->ColorMap,
                                                         opacities,
                                                         token);
  phase1ShadowMapTimer.Stop();
  // FMT_TMR(phase1ShadowMapTimer);
  Phase1Time = phase1ShadowMapTimer.GetElapsedTime();
  this->Profiler->EndFrame();

  LOG::Println0("Phase 2");
  vtkm::cont::Timer phase2MpiTimer;
  phase2MpiTimer.Start();
  MpiTypes MPI_TYPES = ConstructMpiTypes();

  vtkm::cont::Invoker invoker{ Device() };
  const bool useGlancingHits = true;

  vtkm::cont::ArrayHandle<vtkm::Id> hitCounts;
  vtkm::cont::ArrayHandle<vtkm::Id> hitOffsets;
  vtkm::cont::ArrayHandle<TransmittanceRayBlockHit> rayHits;
  GetNonLocalHits<PhotonMapEstimatorType, Device>(transmittanceMapEstimator,
                                                  TheLights,
                                                  *(this->BoundsMap),
                                                  useGlancingHits,
                                                  hitCounts,
                                                  hitOffsets,
                                                  rayHits);

  vtkm::cont::Algorithm::Sort(rayHits, beams::rendering::HitSort());

  std::vector<TransmittanceRayBlockHit> rayHitsV;
  CopyPortalToVector(rayHits.ReadPortal(), rayHitsV);
  ////////////////////////////////
  // Send the sizes of the vectors
  ////////////////////////////////
  std::vector<int> requestNumHits;
  if (mpi->Rank == 0)
  {
    requestNumHits.resize(mpi->Size);
    int count = rayHitsV.size();
    MPI_Gather(&count, 1, MPI_INT, requestNumHits.data(), 1, MPI_INT, 0, mpiComm);
  }
  else
  {
    int count = rayHitsV.size();
    MPI_Gather(&count, 1, MPI_INT, nullptr, 0, MPI_INT, 0, mpiComm);
  }

  ////////////////////////////////
  // Send the vectors
  ////////////////////////////////
  std::vector<TransmittanceRayBlockHit> requestHits;
  if (mpi->Rank == 0)
  {
    std::vector<int> requestHitsOffsets;
    ScanExclusiveVector(requestNumHits, requestHitsOffsets);
    int totalHitsCount = std::accumulate(requestNumHits.begin(), requestNumHits.end(), 0);
    requestHits.resize(totalHitsCount);
    int rayHitsVSize = rayHitsV.size();
    MPI_Gatherv(rayHitsV.data(),
                rayHitsVSize,
                MPI_TYPES.TransmittanceRayBlockHit,
                requestHits.data(),
                requestNumHits.data(),
                requestHitsOffsets.data(),
                MPI_TYPES.TransmittanceRayBlockHit,
                0,
                mpiComm);
  }
  else
  {
    int rayHitsVSize = rayHitsV.size();
    MPI_Gatherv(rayHitsV.data(),
                rayHitsVSize,
                MPI_TYPES.TransmittanceRayBlockHit,
                nullptr,
                nullptr,
                nullptr,
                MPI_TYPES.TransmittanceRayBlockHit,
                0,
                mpiComm);
  }

  std::map<int, std::vector<TransmittanceRayBlockHit>> requestHitsTable;
  if (mpi->Rank == 0)
  {
    for (const TransmittanceRayBlockHit& hit : requestHits)
    {
      int toBlock = hit.BlockId;
      auto& hitsTableEntry = requestHitsTable[toBlock];
      hitsTableEntry.push_back(hit);
    }
  }

  ////////////////////////////////
  // Send the sizes of the vectors
  ////////////////////////////////
  std::vector<int> pullsNumHits;
  int pullNumHits;
  if (mpi->Rank == 0)
  {
    pullsNumHits.resize(mpi->Size);
    for (int i = 0; i < mpi->Size; ++i)
    {
      pullsNumHits[i] = requestHitsTable[i].size();
    }
    MPI_Scatter(pullsNumHits.data(), 1, MPI_INT, &pullNumHits, 1, MPI_INT, 0, mpiComm);
  }
  else
  {
    MPI_Scatter(nullptr, 1, MPI_INT, &pullNumHits, 1, MPI_INT, 0, mpiComm);
  }

  ////////////////////////////////
  // Send the vectors
  ////////////////////////////////
  std::vector<MPI_Request> pullHitsMPIRequests;
  std::vector<MPI_Status> pullHitsMPIStatuses;

  int pullHitsNumMPIRequests = 1;
  if (mpi->Rank == 0)
  {
    pullHitsNumMPIRequests += 1 * mpi->Size;
  }
  pullHitsMPIRequests.resize(pullHitsNumMPIRequests);
  pullHitsMPIStatuses.resize(pullHitsNumMPIRequests);

  if (mpi->Rank == 0)
  {
    int mpiRequestsOffset = 1;
    for (int block = 0; block < mpi->Size; block++)
    {
      int requestIdx = mpiRequestsOffset + block;
      MPI_Isend(requestHitsTable[block].data(),
                requestHitsTable[block].size(),
                MPI_TYPES.TransmittanceRayBlockHit,
                block,
                100,
                mpiComm,
                &pullHitsMPIRequests[requestIdx + 0]);
    }
  }
  std::vector<TransmittanceRayBlockHit> pullHitsV;
  pullHitsV.resize(pullNumHits);
  MPI_Irecv(pullHitsV.data(),
            pullNumHits,
            MPI_TYPES.TransmittanceRayBlockHit,
            0,
            100,
            mpiComm,
            &pullHitsMPIRequests[0]);
  MPI_Waitall(pullHitsNumMPIRequests, pullHitsMPIRequests.data(), pullHitsMPIStatuses.data());

  vtkm::Float32 numSteps = 128.0f;
  vtkm::Float32 stepSize = vtkm::Magnitude(size) / numSteps;

  vtkm::cont::ArrayHandle<TransmittanceRayBlockHit> pullHits =
    vtkm::cont::make_ArrayHandle(pullHitsV, vtkm::CopyFlag::On);
  invoker(
    TransmittanceFetcher2<PhotonMapEstimatorType>{ mpi->Rank, stepSize, transmittanceMapEstimator },
    pullHits);
  CopyPortalToVector(pullHits.ReadPortal(), pullHitsV);

  pullHitsNumMPIRequests = 1;
  if (mpi->Rank == 0)
  {
    pullHitsNumMPIRequests += mpi->Size;
  }
  pullHitsMPIRequests.resize(pullHitsNumMPIRequests);
  pullHitsMPIStatuses.resize(pullHitsNumMPIRequests);
  if (mpi->Rank == 0)
  {
    int mpiRequestsOffset = 1;
    for (int block = 0; block < mpi->Size; block++)
    {
      int requestIdx = mpiRequestsOffset + block;
      MPI_Irecv(requestHitsTable[block].data(),
                requestHitsTable[block].size(),
                MPI_TYPES.TransmittanceRayBlockHit,
                block,
                101,
                mpiComm,
                &pullHitsMPIRequests[requestIdx + 0]);
    }
  }
  MPI_Isend(pullHitsV.data(),
            pullNumHits,
            MPI_TYPES.TransmittanceRayBlockHit,
            0,
            101,
            mpiComm,
            &pullHitsMPIRequests[0]);
  MPI_Waitall(pullHitsNumMPIRequests, pullHitsMPIRequests.data(), pullHitsMPIStatuses.data());

  std::map<int, std::vector<TransmittanceRayBlockHit>> responseHitsTable;
  if (mpi->Rank == 0)
  {
    for (auto i : requestHitsTable)
    {
      auto& hitsV = i.second;
      for (auto& hit : hitsV)
      {
        int fromRank = hit.FromBlockId;
        auto& reponseHitsV = responseHitsTable[fromRank];
        reponseHitsV.push_back(hit);
      }
    }
  }
  std::vector<MPI_Request> responseMPIRequests;
  std::vector<MPI_Status> responseMPIStatuses;
  int numResponses = 1;
  if (mpi->Rank == 0)
  {
    numResponses += mpi->Size;
  }
  responseMPIRequests.resize(numResponses);
  responseMPIStatuses.resize(numResponses);
  if (mpi->Rank == 0)
  {
    int mpiRequestsOffset = 1;
    for (int block = 0; block < mpi->Size; ++block)
    {
      int requestIdx = mpiRequestsOffset + block;
      auto& hits = responseHitsTable[block];
      MPI_Isend(hits.data(),
                hits.size(),
                MPI_TYPES.TransmittanceRayBlockHit,
                block,
                102,
                mpiComm,
                &responseMPIRequests[requestIdx + 0]);
    }
  }

  std::vector<TransmittanceRayBlockHit> rayHits2V;
  rayHits2V.resize(rayHitsV.size());
  MPI_Irecv(rayHits2V.data(),
            rayHits2V.size(),
            MPI_TYPES.TransmittanceRayBlockHit,
            0,
            102,
            mpiComm,
            &responseMPIRequests[0]);
  MPI_Waitall(numResponses, responseMPIRequests.data(), responseMPIStatuses.data());
  phase2MpiTimer.Stop();
  // FMT_TMR(phase2MpiTimer);
  Phase2Time = phase2MpiTimer.GetElapsedTime();

  vtkm::cont::ArrayHandle<TransmittanceRayBlockHit> rayHits2 =
    vtkm::cont::make_ArrayHandle(rayHits2V, vtkm::CopyFlag::On);

  LOG::Println0("Phase 3");
  vtkm::cont::Timer phase3ShadowMapUpdateTimer;
  phase3ShadowMapUpdateTimer.Start();

  vtkm::cont::Algorithm::Sort(rayHits2, beams::rendering::HitSort());
  vtkm::cont::ArrayHandle<vtkm::Float32> newOpacities;
  newOpacities.Allocate(opacities.GetNumberOfValues());
  {
    auto transmittanceP = newOpacities.WritePortal();
    auto offsetP = hitOffsets.ReadPortal();
    auto countsP = hitCounts.ReadPortal();
    auto hitsP = rayHits2.ReadPortal();
    for (int i = 0; i < transmittanceP.GetNumberOfValues(); ++i)
    {
      auto hitCount = countsP.Get(i);
      auto offset = offsetP.Get(i);
      auto opacity = 0.0f;
      for (int j = 0; j < hitCount; ++j)
      {
        auto hit = hitsP.Get(offset + j);
        vtkm::Float32 localOpacity = hit.Opacity;
        opacity = opacity + (1.0f - opacity) * localOpacity;
      }
      transmittanceP.Set(i, opacity);
    }
  }

  vtkm::cont::ArrayHandle<vtkm::Float32> old;
  old.Allocate(opacities.GetNumberOfValues());
  vtkm::cont::Algorithm::Copy(opacities, old);

  transmittanceMapEstimator =
    GenerateEstimator<Device, OracleType, vtkm::Float32>(this->SpatialExtent,
                                                         dims,
                                                         opacityMapDataSet,
                                                         lightRays,
                                                         ScalarRange,
                                                         ScalarField,
                                                         TheLights,
                                                         oracle,
                                                         this->ColorMap,
                                                         newOpacities,
                                                         token);
  auto final = newOpacities;
  phase3ShadowMapUpdateTimer.Stop();
  // FMT_TMR(phase3ShadowMapUpdateTimer);
  Phase3Time = phase3ShadowMapUpdateTimer.GetElapsedTime();

  LOG::Println0("Phase 4");
  vtkm::cont::Timer phase4RenderTimer{ Device() };
  phase4RenderTimer.Start();
  vtkm::Float32 meshEpsilon = this->SpatialExtentMagnitude * 0.0001f;
  vtkm::cont::Timer timer{ Device() };
  timer.Start();
  vtkm::worklet::DispatcherMapField<CalcRayStart> calcRayStartDispatcher(
    CalcRayStart(this->SpatialExtent));
  calcRayStartDispatcher.SetDevice(Device());
  calcRayStartDispatcher.Invoke(
    rays.Dir, rays.MinDistance, rays.Distance, rays.MaxDistance, rays.Origin);

  const bool isAssocPoints = ScalarField->IsPointField();
  LOG::Println0("IsUniform = {}, isAssocPoints = {}", IsUniformDataSet, isAssocPoints);
  if (IsUniformDataSet)
  {
    vtkm::cont::ArrayHandleUniformPointCoordinates vertices;
    vertices =
      CoordinateSystem.GetData().AsArrayHandle<vtkm::cont::ArrayHandleUniformPointCoordinates>();
    UniformLocator<Device> locator(vertices, CellSet, token);

    if (isAssocPoints)
    {
      using SamplerType = Sampler<Device, UniformLocator<Device>, PhotonMapEstimatorType>;
      vtkm::worklet::DispatcherMapField<SamplerType> samplerDispatcher(
        SamplerType(ColorMap,
                    vtkm::Float32(ScalarRange.Min),
                    vtkm::Float32(ScalarRange.Max),
                    SampleDistance,
                    locator,
                    meshEpsilon,
                    transmittanceMapEstimator,
                    this->UseShadowMap,
                    token));
      samplerDispatcher.SetDevice(Device());
      samplerDispatcher.Invoke(
        rays.Dir,
        rays.Origin,
        rays.MinDistance,
        rays.MaxDistance,
        rays.Buffers.at(0).Buffer,
        vtkm::rendering::raytracing::GetScalarFieldArray(*this->ScalarField));
    }
    else
    {
      vtkm::worklet::DispatcherMapField<SamplerCellAssoc<Device, UniformLocator<Device>>>(
        SamplerCellAssoc<Device, UniformLocator<Device>>(ColorMap,
                                                         vtkm::Float32(ScalarRange.Min),
                                                         vtkm::Float32(ScalarRange.Max),
                                                         SampleDistance,
                                                         locator,
                                                         meshEpsilon,
                                                         token))
        .Invoke(rays.Dir,
                rays.Origin,
                rays.MinDistance,
                rays.MaxDistance,
                rays.Buffers.at(0).Buffer,
                vtkm::rendering::raytracing::GetScalarFieldArray(*this->ScalarField));
    }
  }
  else
  {
    CartesianArrayHandle vertices;
    vertices = CoordinateSystem.GetData().AsArrayHandle<CartesianArrayHandle>();
    RectilinearLocator<Device> locator(vertices, CellSet, token);
    if (isAssocPoints)
    {
      using SamplerType = Sampler<Device, RectilinearLocator<Device>, PhotonMapEstimatorType>;
      vtkm::worklet::DispatcherMapField<SamplerType> samplerDispatcher(
        SamplerType(ColorMap,
                    vtkm::Float32(ScalarRange.Min),
                    vtkm::Float32(ScalarRange.Max),
                    SampleDistance,
                    locator,
                    meshEpsilon,
                    transmittanceMapEstimator,
                    this->UseShadowMap,
                    token));
      samplerDispatcher.SetDevice(Device());
      samplerDispatcher.Invoke(
        rays.Dir,
        rays.Origin,
        rays.MinDistance,
        rays.MaxDistance,
        rays.Buffers.at(0).Buffer,
        vtkm::rendering::raytracing::GetScalarFieldArray(*this->ScalarField));
    }
    else
    {
      vtkm::worklet::DispatcherMapField<SamplerCellAssoc<Device, RectilinearLocator<Device>>>
        rectilinearLocatorDispatcher(
          SamplerCellAssoc<Device, RectilinearLocator<Device>>(ColorMap,
                                                               vtkm::Float32(ScalarRange.Min),
                                                               vtkm::Float32(ScalarRange.Max),
                                                               SampleDistance,
                                                               locator,
                                                               meshEpsilon,
                                                               token));
      rectilinearLocatorDispatcher.SetDevice(Device());
      rectilinearLocatorDispatcher.Invoke(
        rays.Dir,
        rays.Origin,
        rays.MinDistance,
        rays.MaxDistance,
        rays.Buffers.at(0).Buffer,
        vtkm::rendering::raytracing::GetScalarFieldArray(*this->ScalarField));
    }
  }

  phase4RenderTimer.Stop();
  // FMT_TMR(phase4RenderTimer);
  Phase4Time = phase4RenderTimer.GetElapsedTime();
  LOG::Println0("Phase 4 End");
}

void LightedVolumeRenderer::SetSampleDistance(const vtkm::Float32& distance)
{
  if (distance <= 0.f)
    throw vtkm::cont::ErrorBadValue("Sample distance must be positive.");
  SampleDistance = distance;
}
} // namespace rendering
} // namespace beams
