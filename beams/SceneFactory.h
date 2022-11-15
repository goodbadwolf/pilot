#ifndef beams_scenefactory_h
#define beams_scenefactory_h

#include "Config.h"
#include "rendering/Scene.h"

#include <memory>

namespace beams
{
struct SceneFactory
{
  static std::shared_ptr<beams::rendering::Scene> CreateFromPreset(const beams::Preset& preset);
}; // struct Scene
} // namespace beams

#endif // beams_scenefactory_h