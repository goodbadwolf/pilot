#ifndef beams_volume_renderer_structured_h
#define beams_volume_renderer_structured_h

#include "../Profiler.h"
#include "BoundsMap.h"
#include "LightCollection.h"

#include "Lights.h"
#include <vtkm/cont/DataSet.h>
#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/raytracing/Ray.h>

#include <memory>
#include <vector>

namespace beams
{
namespace rendering
{
class LightedVolumeRenderer
{
public:
  using DefaultHandle = vtkm::cont::ArrayHandle<vtkm::FloatDefault>;
  using CartesianArrayHandle =
    vtkm::cont::ArrayHandleCartesianProduct<DefaultHandle, DefaultHandle, DefaultHandle>;

  VTKM_CONT
  LightedVolumeRenderer();

  VTKM_CONT
  void EnableCompositeBackground();

  VTKM_CONT
  void DisableCompositeBackground();

  VTKM_CONT
  void SetColorMap(const vtkm::cont::ArrayHandle<vtkm::Vec4f_32>& colorMap);

  VTKM_CONT
  void SetData(const vtkm::cont::CoordinateSystem& coords,
               const vtkm::cont::Field& scalarField,
               const vtkm::cont::CellSetStructured<3>& cellset,
               const vtkm::Range& scalarRange);


  VTKM_CONT
  void Render(vtkm::rendering::raytracing::Ray<vtkm::Float32>& rays);

  VTKM_CONT
  void SetSampleDistance(const vtkm::Float32& distance);

  VTKM_CONT
  void AddLight(std::shared_ptr<Light> light);

  VTKM_CONT
  void ClearLights();

  VTKM_CONT
  void SetUseShadowMap(bool useShadowMap) { this->UseShadowMap = useShadowMap; }

  VTKM_CONT
  void SetShadowMapSize(vtkm::Id3 size) { this->ShadowMapSize = size; }

  VTKM_CONT
  void SetBoundsMap(beams::rendering::BoundsMap* boundsMap) { this->BoundsMap = boundsMap; }

  VTKM_CONT
  void SetProfiler(std::shared_ptr<beams::Profiler> profiler) { this->Profiler = profiler; }

  vtkm::rendering::CanvasRayTracer* Canvas;
  vtkm::Float32 DensityScale;
  beams::rendering::BoundsMap* BoundsMap;
  std::shared_ptr<beams::Profiler> Profiler;

protected:
  template <typename Precision, typename Device>
  VTKM_CONT void RenderOnDevice(vtkm::rendering::raytracing::Ray<Precision>& rays, Device);

  template <typename Precision, typename Device, typename RectilinearOracleType>
  VTKM_CONT RectilinearOracleType BuildRectilinearOracle();

  template <typename Precision>
  struct RenderFunctor;

  LightCollection Lights;
  bool IsSceneDirty;
  bool IsUniformDataSet;
  vtkm::Bounds SpatialExtent;
  vtkm::Float32 SpatialExtentMagnitude;
  vtkm::cont::CoordinateSystem CoordinateSystem;
  vtkm::cont::CellSetStructured<3> CellSet;
  const vtkm::cont::Field* ScalarField;
  vtkm::cont::ArrayHandle<vtkm::Vec4f_32> ColorMap;
  vtkm::Float32 SampleDistance;
  vtkm::Range ScalarRange;
  vtkm::rendering::raytracing::Lights TheLights;
  bool UseShadowMap;
  vtkm::Id3 ShadowMapSize;
};
} // namespace rendering
} // namespace beams

#endif // beams_volume_renderer_structured_h
