#include "SpheresScene.h"
#include "../sources/Spheres.h"
#include "PointLight.h"
#include "mpi/MpiEnv.h"
#include "utils/Fmt.h"

#include <vtkm/Types.h>
#include <vtkm/rendering/CanvasRayTracer.h>

namespace beams
{
namespace rendering
{

std::shared_ptr<beams::rendering::Scene> SpheresScene::CreateFromPreset(const beams::Preset& preset)
{
  vtkm::cont::Timer totalTimer;
  totalTimer.Start();

  auto scene = std::make_shared<beams::rendering::SpheresScene>();
  auto mpi = beams::mpi::MpiEnv::Get();
  if (mpi->Size < 8)
    mpi->ReshapeAsRectangle();
  else
    mpi->ReshapeAsCuboid();

  const vtkm::Id SIZE = std::stoi(preset.DataSetOptions.Params.at("size"));
  const vtkm::Id3 dims{ SIZE };
  const vtkm::Float32 DEFAULT_FIELD_VAL = 0.1f;
  const vtkm::Vec3f_32 origin{ static_cast<vtkm::Float32>(mpi->XRank),
                               static_cast<vtkm::Float32>(mpi->YRank),
                               static_cast<vtkm::Float32>(mpi->ZRank) };
  const vtkm::Vec3f_32 spacing{ 1.0f / static_cast<vtkm::Float32>(dims[0]),
                                1.0f / static_cast<vtkm::Float32>(dims[1]),
                                1.0f / static_cast<vtkm::Float32>(dims[2]) };

  vtkm::cont::Timer createTimer;
  createTimer.Start();
  beams::sources::Spheres source{
    origin, dims, spacing, DEFAULT_FIELD_VAL, beams::sources::CoordinatesType::Rectilinear
  };

  bool shouldAddSpheres = true; //(mpi->Rank == 0);
  bool shouldAddSmallSpheres = false;
  if (shouldAddSpheres)
  {
    const vtkm::Float32 CENTER_SPHERE_RAD = 0.1f;
    const vtkm::Float32 SMALL_SPHERE_RAD = 0.04f;
    const vtkm::Float32 CENTER_SPHERE_VAL = 1.0F;
    const vtkm::Float32 SMALL_FIELD_VALS[] = { 1.0f, 0.8f, 0.6f, 0.4f, 0.2f };
    const vtkm::Vec3f_32 CENTER_SPHERE_OFFSET{ 0.5f, 0.5f /*+ mpi->Rank * 0.1f*/, 0.5f };
    source.AddSphere(
      origin + CENTER_SPHERE_OFFSET, CENTER_SPHERE_RAD, CENTER_SPHERE_VAL, CENTER_SPHERE_VAL);

    if (shouldAddSmallSpheres)
    {
      vtkm::Vec3f partialSphere{ 0.5f * static_cast<vtkm::Float32>(mpi->XLength),
                                 0.5f * static_cast<vtkm::Float32>(mpi->YLength),
                                 0.5f * static_cast<vtkm::Float32>(mpi->ZLength) };
      for (int i = 0; i < 8; ++i)
      {
        vtkm::Vec3f_32 centerPos = origin + CENTER_SPHERE_OFFSET;
        vtkm::Float32 multiplier = (i < 4) ? 1.5f : 5.0f;
        multiplier *= 0.5f + 0.8f * 0.5f;
        int j = i % 4;
        int x = ((j & 1) == 1) ? 1 : -1;
        int z = ((j & 2) == 2) ? 1 : -1;
        int y = (i < 4) ? 1 : -1;

        centerPos[0] += (CENTER_SPHERE_RAD + SMALL_SPHERE_RAD * multiplier) * x;
        centerPos[1] += (CENTER_SPHERE_RAD + SMALL_SPHERE_RAD * multiplier) * y;
        centerPos[2] += (CENTER_SPHERE_RAD + SMALL_SPHERE_RAD * multiplier) * z;
        source.AddSphere(centerPos, SMALL_SPHERE_RAD, SMALL_FIELD_VALS[j], SMALL_FIELD_VALS[j]);
      }
    }
  }

  // Add floor if self rank is bottom layer of ranks
  if (mpi->YRank == 0)
  {
    const vtkm::Float32 FLOOR_FIELD_VAL = 0.7f;
    const vtkm::Float32 FLOOR_HEIGHT = -0.01f;
    source.SetFloor(FLOOR_HEIGHT, FLOOR_FIELD_VAL); // Floor
  }

  scene->DataSet = source.Execute();
  createTimer.Stop();
  scene->FieldName = "sphere";
  scene->Range = vtkm::Range{ 0.0f, 1.0f };
  scene->Id = preset.Id;

  scene->BoundsMap = std::make_shared<beams::rendering::BoundsMap>(scene->DataSet);

  vtkm::cont::ColorTable colorTable{ "cool to warm" };
  colorTable.AddPointAlpha(0.0, 0.0f);
  colorTable.AddPointAlpha(0.1, (shouldAddSpheres ? 0.008f : 0.008f));
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
  scene->ShadowMapSize = preset.OpacityMapOptions.Size;

  totalTimer.Stop();

  auto& blockBounds = scene->BoundsMap->BlockBounds;
  for (vtkm::Id i = 0; i < mpi->Size; ++i)
  {
    Fmt::Println0("Local: Bounds {} => {}", i, blockBounds[i]);
  }

  return scene;
}

beams::Result SpheresScene::Ready()
{
  auto mpi = beams::mpi::MpiEnv::Get();

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
  this->LightPosition = vtkm::Vec3f_32{ 3.1f, 3.55f, 0.5f };
  std::shared_ptr<beams::rendering::Light> light =
    std::make_shared<beams::rendering::PointLight<vtkm::Float32>>(
      this->LightPosition, this->LightColor, 2.0f);
  this->Mapper.ClearLights();
  this->Mapper.AddLight(light);

  return beams::Result::Succeeded();
}
}
} //namespace beams::rendering
