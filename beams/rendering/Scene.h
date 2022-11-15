#ifndef beams_rendering_scene_h
#define beams_rendering_scene_h

#include "../Result.h"
#include "MapperLightedVolume.h"

#include <vtkm/Types.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/rendering/Camera.h>

#include <string>

namespace vtkm
{
namespace rendering
{
class CanvasRayTracer;
}
} // namespace vtkm::rendering

namespace beams
{
namespace rendering
{
struct Scene
{
  Scene() = default;
  virtual ~Scene() = default;

  virtual beams::Result Ready() = 0;

  std::string Id;
  std::string FieldName;
  vtkm::Range Range;
  vtkm::cont::ColorTable ColorTable;
  vtkm::rendering::Camera Camera;
  beams::rendering::MapperLightedVolume Mapper;
  vtkm::cont::DataSet DataSet;
  vtkm::rendering::CanvasRayTracer* Canvas;
  vtkm::Vec3f LightPosition;
  vtkm::Vec3f LightColor;
  vtkm::Id3 ShadowMapSize;
  vtkm::Float32 Azimuth;
  vtkm::Float32 Elevation;
  std::shared_ptr<beams::rendering::BoundsMap> BoundsMap;
}; // struct Scene
}
} // namespace beams::rendering

#endif // beams_rendering_scene_h
