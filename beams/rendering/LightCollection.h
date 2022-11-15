#ifndef beams_rendering_lightcollection_h
#define beams_rendering_lightcollection_h

#include "Light.h"

#include <vtkm/Types.h>
#include <vtkm/cont/Token.h>

#include <memory>
#include <vector>

namespace beams
{
namespace rendering
{
template <typename Device>
struct LightCollectionExec;

struct LightCollection
{
public:
  LightCollection() = default;
  ~LightCollection() = default;

  void AddLight(std::shared_ptr<Light> light);

  template <typename Device>
  beams::rendering::LightCollectionExec<Device> PrepareForExecution(Device,
                                                                    vtkm::cont::Token& token) const
  {
    return LightCollectionExec<Device>(token);
  }

private:
  std::vector<std::shared_ptr<Light>> Lights;
}; // struct LightCollection

} // namespace rendering
} // namespace beams

#endif // beams_rendering_lightcollection_h
