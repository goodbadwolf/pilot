#ifndef beams_rendering_bonsaiscene_h
#define beams_rendering_bonsaiscene_h

#include "../Config.h"
#include "FileSceneBase.h"

#include <memory>

namespace beams
{
namespace rendering
{
struct BonsaiScene : public FileSceneBase
{
  BonsaiScene() {}
  virtual ~BonsaiScene() {}

  std::string GetFieldName(const beams::Preset& preset) override;

  void ShapeMpiTopology(std::shared_ptr<beams::mpi::MpiEnv> mpiEnv) override;

  static std::shared_ptr<beams::rendering::Scene> CreateFromPreset(const beams::Preset& preset);
}; // struct BonsaiScene
}
} // namespace beams::rendering

#endif // beams_rendering_bonsaiscene_h