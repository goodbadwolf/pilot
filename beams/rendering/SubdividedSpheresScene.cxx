#include "SubdividedSpheresScene.h"
#include "../sources/Spheres.h"
#include "PointLight.h"
#include <pilot/Logger.h>
#include <pilot/mpi/Environment.h>

#include <vtkm/Types.h>
#include <vtkm/rendering/CanvasRayTracer.h>

namespace beams
{
namespace rendering
{

std::shared_ptr<beams::rendering::Scene> SubdividedSpheresScene::CreateFromPreset(
  const beams::Preset& preset)
{
  vtkm::cont::Timer totalTimer;
  totalTimer.Start();

  auto scene = std::make_shared<beams::rendering::SubdividedSpheresScene>();
  auto mpi = pilot::mpi::Environment::Get();
  if (mpi->Size < 8)
    mpi->ReshapeAsRectangle();
  else
    mpi->ReshapeAsCuboid();

  const vtkm::Id3 totalDims{ std::stoi(preset.DataSetOptions.Params.at("size")) };
  const vtkm::Id3 dims{ totalDims[0] / mpi->XLength,
                        totalDims[1] / mpi->YLength,
                        totalDims[2] / mpi->ZLength };
  const vtkm::Vec3f_32 spacing{ 1.0f / static_cast<vtkm::Float32>(totalDims[0]),
                                1.0f / static_cast<vtkm::Float32>(totalDims[1]),
                                1.0f / static_cast<vtkm::Float32>(totalDims[2]) };

  const vtkm::Vec3f_32 lengths{
    static_cast<vtkm::Float32>(dims[0]) / static_cast<vtkm::Float32>(totalDims[0]),
    static_cast<vtkm::Float32>(dims[1]) / static_cast<vtkm::Float32>(totalDims[1]),
    static_cast<vtkm::Float32>(dims[2]) / static_cast<vtkm::Float32>(totalDims[2])
  };
  const vtkm::Vec3f_32 origin{ static_cast<vtkm::Float32>(mpi->XRank) * lengths[0],
                               static_cast<vtkm::Float32>(mpi->YRank) * lengths[1],
                               static_cast<vtkm::Float32>(mpi->ZRank) * lengths[2] };
  const vtkm::Float32 DEFAULT_FIELD_VAL = 0.1f;

  vtkm::cont::Timer createTimer;
  createTimer.Start();
  beams::sources::Spheres source{
    origin, dims, spacing, DEFAULT_FIELD_VAL, beams::sources::CoordinatesType::Rectilinear
  };

  const vtkm::Float32 CENTER_SPHERE_RAD = 0.45f;
  const vtkm::Float32 CENTER_SPHERE_VAL = 1.0f;
  const vtkm::Vec3f_32 CENTER_SPHERE_OFFSET{ 0.5f, 0.5f, 0.5f };
  source.AddSphere(CENTER_SPHERE_OFFSET, CENTER_SPHERE_RAD, CENTER_SPHERE_VAL, CENTER_SPHERE_VAL);
  /*
  source.AddSphere(CENTER_SPHERE_OFFSET +
                     vtkm::Vec3f_32{ -0.5f * CENTER_SPHERE_RAD, 0.5f * CENTER_SPHERE_RAD, 0.0f },
                   CENTER_SPHERE_RAD,
                   CENTER_SPHERE_VAL,
                   CENTER_SPHERE_VAL);
  */
  scene->DataSet = source.Execute();
  createTimer.Stop();
  scene->FieldName = "sphere";
  scene->Range = vtkm::Range{ 0.0f, 1.0f };
  scene->Id = preset.Id;

  scene->BoundsMap = std::make_shared<beams::rendering::BoundsMap>(scene->DataSet);

  vtkm::cont::ColorTable colorTable{ "cool to warm" };
  colorTable.AddPointAlpha(0.0, 0.0f);
  colorTable.AddPointAlpha(0.1, 0.008f);
  colorTable.AddPointAlpha(0.2, 0.2f);
  colorTable.AddPointAlpha(0.3, 1.0f);
  colorTable.AddPointAlpha(0.4, 0.1f);
  colorTable.AddPointAlpha(0.5, 1.0f);
  colorTable.AddPointAlpha(0.6, 0.6f);
  colorTable.AddPointAlpha(0.7, 0.7f);
  colorTable.AddPointAlpha(0.8, 1.0f);
  colorTable.AddPointAlpha(0.9, 0.7f);
  colorTable.AddPointAlpha(1.0, 1.0f);
  scene->ColorTable = colorTable;

  scene->Azimuth = preset.CameraOptions.Azimuth;
  scene->Elevation = preset.CameraOptions.Elevation;

  scene->LightColor = preset.LightOptions.Lights[0].Color;
  scene->ShadowMapSize = preset.OpacityMapOptions.CalculateSize(dims);
  LOG::Println0("Opacity map size = {}", scene->ShadowMapSize);

  totalTimer.Stop();

  auto& blockBounds = scene->BoundsMap->BlockBounds;
  for (vtkm::Id i = 0; i < mpi->Size; ++i)
  {
    LOG::Println0("Local: Bounds {} => {}", i, blockBounds[i]);
  }

  return scene;
}

beams::Result SubdividedSpheresScene::Ready()
{
  auto mpi = pilot::mpi::Environment::Get();

  vtkm::rendering::Color background(1.0f, 1.0f, 1.0f, 1.0f);
  vtkm::rendering::Color foreground(0.0f, 0.0f, 0.0f, 1.0f);
  this->Canvas->SetBackgroundColor(background);
  this->Canvas->SetForegroundColor(foreground);
  this->Canvas->Clear();
  this->Camera = vtkm::rendering::Camera();
  this->Camera.ResetToBounds(this->BoundsMap->GlobalBounds);
  this->Camera.Azimuth(this->Azimuth);
  this->Camera.Elevation(this->Elevation);

  this->Mapper.SetCompositeBackground(true);
  this->Mapper.SetCanvas(this->Canvas);
  this->Mapper.SetActiveColorTable(this->ColorTable);

  this->Mapper.SetDensityScale(1.0f);
  this->Mapper.SetBoundsMap(this->BoundsMap.get());
  this->Mapper.SetUseShadowMap(true);
  this->Mapper.SetShadowMapSize(this->ShadowMapSize);
  this->LightPosition = vtkm::Vec3f_32{ 0.5f, 5.0f, 0.5f };
  std::shared_ptr<beams::rendering::Light> light =
    std::make_shared<beams::rendering::PointLight<vtkm::Float32>>(
      this->LightPosition, this->LightColor, 2.0f);
  this->Mapper.ClearLights();
  this->Mapper.AddLight(light);

  return beams::Result::Succeeded();
}
}
} //namespace beams::rendering
