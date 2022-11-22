#include "EngineScene.h"
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
void EngineScene::ShapeMpiTopology(std::shared_ptr<pilot::mpi::Environment> mpiEnv)
{
  mpiEnv->ReshapeAsLine();
}

std::string EngineScene::GetFieldName(const beams::Preset& vtkmNotUsed(preset))
{
  return "nrrd67322";
}

std::shared_ptr<beams::rendering::Scene> EngineScene::CreateFromPreset(const beams::Preset& preset)
{
  auto scenePtr = new beams::rendering::EngineScene();
  auto scene = std::shared_ptr<beams::rendering::EngineScene>(scenePtr);
  scene->Init(preset);
  return scene;
}

/*
beams::Result EngineScene::Ready()
{
  auto result = FileSceneBase::Ready();
  this->LightPosition = vtkm::Vec3f_32{ 0.7f, 1.2f, 1.5f };
  std::shared_ptr<beams::rendering::Light> light =
    std::make_shared<beams::rendering::PointLight<vtkm::Float32>>(
      this->LightPosition, this->LightColor, 2.0f);
  this->Mapper.ClearLights();
  this->Mapper.AddLight(light);
  return result;
}
*/

}
} //namespace beams::rendering
