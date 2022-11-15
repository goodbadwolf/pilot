#ifndef beams_rendering_vortexpatchscene_h
#define beams_rendering_vortexpatchscene_h

#include "../Config.h"
#include "Scene.h"

#include <memory>

namespace beams
{
namespace rendering
{
struct VortexPatchScene : public Scene
{
  beams::Result Ready() override;

  static std::shared_ptr<beams::rendering::Scene> CreateFromPreset(const beams::Preset& preset);
}; // struct VortexPatchScene
}
} // namespace beams::rendering

#endif // beams_rendering_vortexpatchscene_h