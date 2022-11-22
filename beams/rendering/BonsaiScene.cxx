#include "BonsaiScene.h"
#include "../sources/Spheres.h"
#include "PointLight.h"
#include <pilot/Logger.h>
#include <pilot/mpi/Environment.h>

#include <vtkm/Types.h>
#include <vtkm/io/FileUtils.h>
#include <vtkm/io/VTKDataSetReader.h>
#include <vtkm/rendering/CanvasRayTracer.h>

namespace beams
{
namespace rendering
{
void BonsaiScene::ShapeMpiTopology(std::shared_ptr<pilot::mpi::Environment> mpiEnv)
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
/*
0   -  30 => 0.0000 - 0.1176,
35  -  45 => 0.1367 - 0.1758,
120 - 160 => 0.4688 - 0.6250,
200 - 220 => 0.7813 - 0.8594
*/
} //namespace beams::rendering
