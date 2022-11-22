#ifndef beams_rendering_filescenebase_h
#define beams_rendering_filescenebase_h

#include "../Config.h"
#include "Scene.h"
#include <pilot/mpi/Environment.h>

#include <memory>

namespace beams
{
namespace rendering
{
struct FileSceneBase : public Scene
{
  FileSceneBase() {}
  virtual ~FileSceneBase() {}

  beams::Result Ready() override;

  virtual void Init(const beams::Preset& preset);

  virtual vtkm::cont::DataSet GetDataSet(const beams::Preset& preset);

  virtual std::string GetFieldName(const beams::Preset& preset);

  virtual void ShapeMpiTopology(std::shared_ptr<pilot::mpi::Environment>) {}
}; // struct FileSceneBase
}
} // namespace beams::rendering

#endif // beams_rendering_filescenebase_h