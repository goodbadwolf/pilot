#include "FootScene.h"
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
void FootScene::ShapeMpiTopology(std::shared_ptr<pilot::mpi::Environment> mpiEnv)
{
  mpiEnv->ReshapeAsLine();
}

std::string FootScene::GetFieldName(const beams::Preset& vtkmNotUsed(preset))
{
  return "nrrd11550";
}

std::shared_ptr<beams::rendering::Scene> FootScene::CreateFromPreset(const beams::Preset& preset)
{
  auto scenePtr = new beams::rendering::FootScene();
  auto scene = std::shared_ptr<beams::rendering::FootScene>(scenePtr);
  scene->Init(preset);
  return scene;
}
}
} //namespace beams::rendering
