#ifndef beams_rendering_enginescene_h
#define beams_rendering_enginescene_h

#include "../Config.h"
#include "FileSceneBase.h"

#include <memory>

namespace beams
{
namespace rendering
{
struct EngineScene : public FileSceneBase
{
  EngineScene() {}
  virtual ~EngineScene() {}

  // beams::Result Ready() override;

  std::string GetFieldName(const beams::Preset& preset) override;

  void ShapeMpiTopology(std::shared_ptr<pilot::mpi::Environment> mpiEnv) override;

  static std::shared_ptr<beams::rendering::Scene> CreateFromPreset(const beams::Preset& preset);
}; // struct EngineScene
}
} // namespace beams::rendering

#endif // beams_rendering_enginescene_h