#include "SceneFactory.h"
#include "rendering/BonsaiScene.h"
#include "rendering/EngineScene.h"
#include "rendering/FootScene.h"
#include "rendering/HeadScene.h"
#include "rendering/SpheresScene.h"
#include "rendering/SubdividedSpheresScene.h"
#include "rendering/VortexPatchScene.h"
#include <pilot/Logger.h>

namespace beams
{
std::shared_ptr<beams::rendering::Scene> SceneFactory::CreateFromPreset(const beams::Preset& preset)
{
  if (preset.DataSetOptions.Factory == "spheres")
  {
    return beams::rendering::SpheresScene::CreateFromPreset(preset);
  }
  else if (preset.DataSetOptions.Factory == "subdivided_spheres")
  {
    return beams::rendering::SubdividedSpheresScene::CreateFromPreset(preset);
  }
  else if (preset.DataSetOptions.Factory == "vortex_patch")
  {
    return beams::rendering::VortexPatchScene::CreateFromPreset(preset);
  }
  else if (preset.DataSetOptions.Factory == "engine")
  {
    return beams::rendering::EngineScene::CreateFromPreset(preset);
  }
  else if (preset.DataSetOptions.Factory == "bonsai")
  {
    return beams::rendering::BonsaiScene::CreateFromPreset(preset);
  }
  else if (preset.DataSetOptions.Factory == "foot")
  {
    return beams::rendering::FootScene::CreateFromPreset(preset);
  }
  else if (preset.DataSetOptions.Factory == "head")
  {
    return beams::rendering::HeadScene::CreateFromPreset(preset);
  }
  else
  {
    LOG::Println("Trying to load preset from invalid dataset type");
    return nullptr;
  }
}
} // namespace beams
