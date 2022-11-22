#ifndef beams_rendering_footscene_h
#define beams_rendering_footscene_h

#include "../Config.h"
#include "FileSceneBase.h"

#include <memory>

namespace beams
{
namespace rendering
{
struct FootScene : public FileSceneBase
{
  FootScene() {}
  virtual ~FootScene() {}

  std::string GetFieldName(const beams::Preset& preset) override;

  void ShapeMpiTopology(std::shared_ptr<pilot::mpi::Environment> mpiEnv) override;

  static std::shared_ptr<beams::rendering::Scene> CreateFromPreset(const beams::Preset& preset);
}; // struct FootScene
}
} // namespace beams::rendering

#endif // beams_rendering_footscene_h