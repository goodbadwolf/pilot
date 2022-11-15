#include "FileSceneBase.h"
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
bool string_replace(std::string& str, const std::string& from, const std::string& to)
{
  std::size_t pos = str.find(from);
  if (pos == std::string::npos)
  {
    return false;
  }
  str.replace(pos, from.length(), to);
  return true;
}

void string_replace_all(std::string& str, const std::string& from, const std::string& to)
{
  while (string_replace(str, from, to))
    ;
}

vtkm::cont::DataSet FileSceneBase::GetDataSet(const beams::Preset& preset)
{
  auto mpi = beams::mpi::MpiEnv::Get();
  auto fileName = preset.DataSetOptions.Params.at("filePattern");
  string_replace_all(fileName, "%s", std::to_string(mpi->Size));
  string_replace_all(fileName, "%r", std::to_string(mpi->Rank));
  Fmt::Print("Trying to load {}", fileName);
  vtkm::io::VTKDataSetReader source(fileName);
  return source.ReadDataSet();
}

std::string FileSceneBase::GetFieldName(const beams::Preset& vtkmNotUsed(preset))
{
  return "";
}

void FileSceneBase::Init(const beams::Preset& preset)
{
  auto mpi = beams::mpi::MpiEnv::Get();
  this->ShapeMpiTopology(mpi);
  this->DataSet = this->GetDataSet(preset);
  this->FieldName = this->GetFieldName(preset);
  this->Range = vtkm::Range{ 0.0f, 255.0f };
  this->Id = preset.Id;

  this->BoundsMap = std::make_shared<beams::rendering::BoundsMap>(this->DataSet);

  vtkm::cont::ColorTable colorTable; //{ "cool to warm" };
  colorTable.AddPointAlpha(0.0, 0.0f);
  colorTable.AddPointAlpha(0.199, 0.0f);
  colorTable.AddPointAlpha(0.2, 0.1f);
  colorTable.AddPointAlpha(1.0, 1.0f);

  this->ColorTable = colorTable;

  this->Azimuth = preset.CameraOptions.Azimuth;
  this->Elevation = preset.CameraOptions.Elevation;

  this->LightColor = preset.LightOptions.Lights[0].Color;
  this->ShadowMapSize = { 32, 32, 16 };
  Fmt::Println0("Opacity map size = {}", this->ShadowMapSize);

  auto& blockBounds = this->BoundsMap->BlockBounds;
  for (vtkm::Id i = 0; i < mpi->Size; ++i)
  {
    Fmt::Println0("Local: Bounds {} => {}", i, blockBounds[i]);
  }
}

beams::Result FileSceneBase::Ready()
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
  // this->LightPosition = vtkm::Vec3f_32{ 0.5f, 0.1f, 0.5f };
  this->LightPosition = vtkm::Vec3f_32{ 0.8f, 1.2f, 1.5f };
  std::shared_ptr<beams::rendering::Light> light =
    std::make_shared<beams::rendering::PointLight<vtkm::Float32>>(
      this->LightPosition, this->LightColor, 2.0f);
  this->Mapper.ClearLights();
  this->Mapper.AddLight(light);

  return beams::Result::Succeeded();
}
}
} //namespace beams::rendering
