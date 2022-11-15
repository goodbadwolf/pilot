#include "LightCollection.h"

namespace beams
{
namespace rendering
{
void LightCollection::AddLight(std::shared_ptr<Light> light)
{
  this->Lights.push_back(light);
}
} // namespace rendering
} // namespace beams