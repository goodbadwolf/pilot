#ifndef beams_rendering_headscene_h
#define beams_rendering_headscene_h

#include "../Config.h"
#include "FileSceneBase.h"

#include <memory>

namespace beams
{
namespace rendering
{
struct HeadScene : public FileSceneBase
{
  HeadScene() {}
  virtual ~HeadScene() {}

  std::string GetFieldName(const beams::Preset& preset) override;

  void ShapeMpiTopology(std::shared_ptr<pilot::mpi::Environment> mpiEnv) override;

  static std::shared_ptr<beams::rendering::Scene> CreateFromPreset(const beams::Preset& preset);
}; // struct HeadScene
}
} // namespace beams::rendering

#endif // beams_rendering_headscene_h