#include "MapperLightedVolume.h"
#include "../Profiler.h"
#include "LightedVolumeRenderer.h"
#include <pilot/Logger.h>
#include <pilot/mpi/Environment.h>

#include <vtkm/cont/Timer.h>
#include <vtkm/cont/TryExecute.h>
#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/raytracing/Camera.h>
#include <vtkm/rendering/raytracing/Logger.h>
#include <vtkm/rendering/raytracing/RayOperations.h>
#include <vtkm/thirdparty/diy/diy.h>
#include <vtkm/thirdparty/diy/mpi-cast.h>

#include <mpi.h>
#include <sstream>

#define DEFAULT_SAMPLE_DISTANCE -1.f

extern vtkm::Float64 Phase5Time;

namespace beams
{
namespace rendering
{
struct MapperLightedVolume::InternalsType
{
  vtkm::rendering::CanvasRayTracer* Canvas;
  vtkm::Float32 SampleDistance;
  bool CompositeBackground;
  beams::rendering::LightedVolumeRenderer Tracer;
  beams::rendering::BoundsMap* BoundsMap;
  std::shared_ptr<beams::Profiler> Profiler;

  VTKM_CONT
  InternalsType()
    : Canvas(nullptr)
    , SampleDistance(DEFAULT_SAMPLE_DISTANCE)
    , CompositeBackground(true)
    , BoundsMap(nullptr)
    , Profiler(nullptr)
  {
  }
};

MapperLightedVolume::MapperLightedVolume()
  : Internals(new InternalsType)
{
}

MapperLightedVolume::~MapperLightedVolume() {}

void MapperLightedVolume::SetCanvas(vtkm::rendering::Canvas* canvas)
{
  if (canvas != nullptr)
  {
    this->Internals->Canvas = dynamic_cast<vtkm::rendering::CanvasRayTracer*>(canvas);

    if (this->Internals->Canvas == nullptr)
    {
      throw vtkm::cont::ErrorBadValue("Ray Tracer: bad canvas type. Must be CanvasRayTracer");
    }
  }
  else
  {
    this->Internals->Canvas = nullptr;
  }
}

vtkm::rendering::Canvas* MapperLightedVolume::GetCanvas() const
{
  return this->Internals->Canvas;
}

void MapperLightedVolume::AddLight(std::shared_ptr<Light> light)
{
  this->Internals->Tracer.AddLight(light);
}

void MapperLightedVolume::ClearLights()
{
  this->Internals->Tracer.ClearLights();
}

void MapperLightedVolume::SetDensityScale(vtkm::Float32 densityScale)
{
  this->Internals->Tracer.DensityScale = densityScale;
}

void MapperLightedVolume::SetBoundsMap(beams::rendering::BoundsMap* boundsMap)
{
  this->Internals->BoundsMap = boundsMap;
}

void MapperLightedVolume::SetUseShadowMap(bool useShadowMap)
{
  this->Internals->Tracer.SetUseShadowMap(useShadowMap);
}

void MapperLightedVolume::SetShadowMapSize(vtkm::Id3 size)
{
  this->Internals->Tracer.SetShadowMapSize(size);
}

void WriteCanvas(vtkm::rendering::CanvasRayTracer* canvas)
{
  auto mpi = pilot::mpi::Environment::Get();
  std::stringstream ss;
  ss << "partial_" << mpi->Rank << ".png";
  canvas->SaveAs(ss.str());
}

void MapperLightedVolume::RenderCells(const vtkm::cont::UnknownCellSet& cellset,
                                      const vtkm::cont::CoordinateSystem& coords,
                                      const vtkm::cont::Field& scalarField,
                                      const vtkm::cont::ColorTable& vtkmNotUsed(colorTable),
                                      const vtkm::rendering::Camera& camera,
                                      const vtkm::Range& scalarRange)
{
  if (!cellset.IsType<vtkm::cont::CellSetStructured<3>>())
  {
    std::stringstream msg;
    std::string theType = typeid(cellset).name();
    msg << "Mapper volume: cell set type not currently supported\n";
    msg << "Type : " << theType << std::endl;
    throw vtkm::cont::ErrorBadValue(msg.str());
  }
  else
  {
    vtkm::rendering::raytracing::Logger* logger =
      vtkm::rendering::raytracing::Logger::GetInstance();
    logger->OpenLogEntry("mapper_volume");
    vtkm::cont::Timer tot_timer;
    tot_timer.Start();
    vtkm::cont::Timer timer;

    auto& tracer = this->Internals->Tracer;
    tracer.Canvas = this->Internals->Canvas;

    vtkm::rendering::raytracing::Camera rayCamera;
    vtkm::rendering::raytracing::Ray<vtkm::Float32> rays;

    vtkm::Int32 width = (vtkm::Int32)this->Internals->Canvas->GetWidth();
    vtkm::Int32 height = (vtkm::Int32)this->Internals->Canvas->GetHeight();

    rayCamera.SetParameters(camera, width, height);

    rayCamera.CreateRays(rays, coords.GetBounds());
    rays.Buffers.at(0).InitConst(0.f);
    vtkm::rendering::raytracing::RayOperations::MapCanvasToRays(
      rays, camera, *this->Internals->Canvas);


    if (this->Internals->SampleDistance != DEFAULT_SAMPLE_DISTANCE)
    {
      tracer.SetSampleDistance(this->Internals->SampleDistance);
    }

    tracer.SetData(
      coords, scalarField, cellset.AsCellSet<vtkm::cont::CellSetStructured<3>>(), scalarRange);
    tracer.SetColorMap(this->ColorMap);
    tracer.SetBoundsMap(this->Internals->BoundsMap);

    tracer.Render(rays);

    vtkm::cont::Timer phase5CompositeTimer;
    phase5CompositeTimer.Start();
    this->Internals->Canvas->WriteToCanvas(rays, rays.Buffers.at(0).Buffer, camera);

    /*
    vtkm::rendering::CanvasRayTracer tmpCanvas(*this->Internals->Canvas);
    if (this->Internals->CompositeBackground)
    {
      tmpCanvas.BlendBackground();
    }
    auto mpi = pilot::mpi::Environment::Get();
    std::string name = "sphere_p_" + std::to_string(mpi->Rank) + ".png";
    Fmt::Println("Saving: {}", name);
    tmpCanvas.SaveAs(name);
    */

    this->GlobalComposite(camera);

    if (this->Internals->CompositeBackground)
    {
      this->Internals->Canvas->BlendBackground();
    }
    phase5CompositeTimer.Stop();
    Phase5Time = phase5CompositeTimer.GetElapsedTime();
    // FMT_TMR0(phase5CompositeTimer);
  }
}

struct DepthOrder
{
  int Rank;
  vtkm::Float32 Depth;
};

template <typename Portal, typename T>
void CopyIntoVec(const Portal& portal, std::vector<T>& vec)
{
  for (int i = 0; i < portal.GetNumberOfValues(); ++i)
  {
    vec.push_back(portal.Get(i));
  }
}

bool IsTransparentColor(const vtkm::Vec4f_32 color)
{
  return color[3] == 0.0f;
}

void MapperLightedVolume::GlobalComposite(const vtkm::rendering::Camera& camera)
{
  auto mpi = pilot::mpi::Environment::Get();
  auto comm = mpi->Comm;
  MPI_Comm mpiComm = vtkmdiy::mpi::mpi_cast(comm->handle());

  std::vector<DepthOrder> depthOrders;
  auto cameraPos = camera.GetPosition();
  for (int i = 0; i < mpi->Size; ++i)
  {
    auto& bounds = this->Internals->BoundsMap->BlockBounds[i];
    auto center = bounds.Center();
    vtkm::Vec3f_32 center_32{ vtkm::Float32(center[0]),
                              vtkm::Float32(center[1]),
                              vtkm::Float32(center[2]) };
    vtkm::Float32 depth = vtkm::Magnitude(cameraPos - center_32);
    depthOrders.push_back({ .Rank = i, .Depth = depth });
  }

  std::sort(depthOrders.begin(),
            depthOrders.end(),
            [](const DepthOrder& l, const DepthOrder& r) -> bool { return l.Depth < r.Depth; });
  std::vector<int> depthRanks;
  for (auto& depth : depthOrders)
  {
    depthRanks.push_back(depth.Rank);
  }

  auto colorBuffer = this->Internals->Canvas->GetColorBuffer();
  int numPixels = colorBuffer.GetNumberOfValues();
  int totalNumPixels = mpi->Size * numPixels;

  std::vector<vtkm::Vec4f_32> gatheredColors;

  MPI_Datatype MPI_VEC4F32;
  MPI_Type_contiguous(4, MPI_FLOAT, &MPI_VEC4F32);
  MPI_Type_commit(&MPI_VEC4F32);

  std::vector<vtkm::Vec4f_32> colorVals;
  CopyIntoVec(colorBuffer.ReadPortal(), colorVals);
  if (mpi->Rank == 0)
  {
    gatheredColors.resize(totalNumPixels);
    MPI_Gather(colorVals.data(),
               numPixels,
               MPI_VEC4F32,
               gatheredColors.data(),
               numPixels,
               MPI_VEC4F32,
               0,
               mpiComm);
  }
  else
  {
    MPI_Gather(colorVals.data(), numPixels, MPI_VEC4F32, nullptr, 0, MPI_VEC4F32, 0, mpiComm);
  }

  if (mpi->Rank != 0)
    return;

  std::vector<vtkm::Vec4f_32> finalBuffer;
  finalBuffer.resize(numPixels);
  int offsetStart = depthRanks[0] * numPixels;
  int offsetEnd = offsetStart + numPixels;
  for (int i = offsetStart; i < offsetEnd; ++i)
  {
    finalBuffer[i - offsetStart] = gatheredColors[i];
  }

  for (std::size_t i = 1; i < depthRanks.size(); ++i)
  {
    int offset = depthRanks[i] * numPixels;
    for (int j = 0; j < numPixels; ++j)
    {
      auto a = finalBuffer[j];
      auto b = gatheredColors[j + offset];

      a[0] = a[0] + b[0] * (1.0f - a[3]);
      a[1] = a[1] + b[1] * (1.0f - a[3]);
      a[2] = a[2] + b[2] * (1.0f - a[3]);
      a[3] = a[3] + b[3] * (1.0f - a[3]);

      finalBuffer[j] = a;
    }
  }

  vtkm::cont::Algorithm::Copy(vtkm::cont::make_ArrayHandle(finalBuffer, vtkm::CopyFlag::Off),
                              colorBuffer);
}

void MapperLightedVolume::SetProfiler(std::shared_ptr<beams::Profiler> profiler)
{
  this->Internals->Profiler = profiler;
  this->Internals->Tracer.SetProfiler(profiler);
}

vtkm::rendering::Mapper* MapperLightedVolume::NewCopy() const
{
  return new beams::rendering::MapperLightedVolume(*this);
}

void MapperLightedVolume::SetSampleDistance(const vtkm::Float32 sampleDistance)
{
  this->Internals->SampleDistance = sampleDistance;
}

void MapperLightedVolume::SetCompositeBackground(const bool compositeBackground)
{
  this->Internals->CompositeBackground = compositeBackground;
}
} // namespace rendering
} // namespace beams
