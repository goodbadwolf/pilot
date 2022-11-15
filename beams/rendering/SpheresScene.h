#ifndef beams_rendering_spheresscene_h
#define beams_rendering_sphreesscene_h

#include "../Config.h"
#include "Scene.h"

#include <memory>

namespace beams
{
namespace rendering
{
struct SpheresScene : public Scene
{
  beams::Result Ready() override;

  static std::shared_ptr<beams::rendering::Scene> CreateFromPreset(const beams::Preset& preset);
}; // struct SpheresScene
}
} // namespace beams::rendering

#endif // beams_rendering_spheresscene_h
