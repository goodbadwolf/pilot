#ifndef beams_rendering_pointlight_h
#define beams_rendering_pointlight_h

#include "Light.h"

#include <vtkm/Types.h>

#include <memory>
#include <vector>

namespace beams
{
namespace rendering
{
template <typename Precision>
struct PointLight : public Light
{
  using Vec3 = vtkm::Vec<Precision, 3>;

  PointLight(Vec3 position, Vec3 color, Precision intensity)
    : Position(position)
    , Color(color)
    , Intensity(intensity)
  {
  }

  Vec3 Position;
  Vec3 Color;
  Precision Intensity;
}; // struct PointLight
} // namespace rendering
} // namespace beams

#endif // beams_rendering_pointlight_h
