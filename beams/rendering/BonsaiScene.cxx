#include "BonsaiScene.h"
#include "../sources/Spheres.h"
#include "PointLight.h"
#include "mpi/MpiEnv.h"
#include "utils/Fmt.h"

#include <vtkm/Types.h>
#include <vtkm/io/FileUtils.h>
#include <vtkm/io/VTKDataSetReader.h>
#include <vtkm/rendering/CanvasRayTracer.h>

namespace beams
{
namespace rendering
{
void BonsaiScene::ShapeMpiTopology(std::shared_ptr<beams::mpi::MpiEnv> mpiEnv)
{
  mpiEnv->ReshapeAsLine();
}

std::string BonsaiScene::GetFieldName(const beams::Preset& vtkmNotUsed(preset))
{
  return "nrrd52089";
}

std::shared_ptr<beams::rendering::Scene> BonsaiScene::CreateFromPreset(const beams::Preset& preset)
{
  auto scenePtr = new beams::rendering::BonsaiScene();
  auto scene = std::shared_ptr<beams::rendering::BonsaiScene>(scenePtr);
  scene->Init(preset);
  return scene;
}

}
} //namespace beams::rendering
