#include "HeadScene.h"
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
void HeadScene::ShapeMpiTopology(std::shared_ptr<pilot::mpi::Environment> mpiEnv)
{
  mpiEnv->ReshapeAsLine();
}

std::string HeadScene::GetFieldName(const beams::Preset& vtkmNotUsed(preset))
{
  return "nrrd95218";
}

std::shared_ptr<beams::rendering::Scene> HeadScene::CreateFromPreset(const beams::Preset& preset)
{
  auto scenePtr = new beams::rendering::HeadScene();
  auto scene = std::shared_ptr<beams::rendering::HeadScene>(scenePtr);
  scene->Init(preset);

  vtkm::cont::ColorTable colorTable = vtkm::cont::ColorTable("head_mri");
  colorTable.SetClampingOff();
  colorTable.SetColorSpace(vtkm::ColorSpace::RGB);
  colorTable.RescaleToRange(vtkm::Range(0.0, 255.0));
  colorTable.AddPoint(0, { 0.91f, 0.7f, 0.61f });
  colorTable.AddPoint(80, { 0.91f, 0.7f, 0.61f });
  colorTable.AddPoint(82, { 1.0f, 1.0f, 0.85f });
  colorTable.AddPoint(255, { 1.0f, 1.0f, 0.85f });
  colorTable.AddPointAlpha(0.0f, 0.008f);
  colorTable.AddPointAlpha(40.0f, 0.008f);
  colorTable.AddPointAlpha(60.0f, 0.2f);
  colorTable.AddPointAlpha(63.0f, 0.05f);
  colorTable.AddPointAlpha(80.0f, 0.0f);
  colorTable.AddPointAlpha(82.0f, 0.9f);
  colorTable.AddPointAlpha(255.0f, 1.0f);
  scene->ColorTable = colorTable;

  scene->ShadowMapSize = { 128, 128, 128 };
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
