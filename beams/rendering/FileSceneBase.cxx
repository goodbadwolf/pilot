#include "FileSceneBase.h"
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
  auto mpi = pilot::mpi::Environment::Get();
  auto fileName = preset.DataSetOptions.Params.at("filePattern");
  string_replace_all(fileName, "%s", std::to_string(mpi->Size));
  string_replace_all(fileName, "%r", std::to_string(mpi->Rank));
  LOG::Println("Trying to load {}", fileName);
  vtkm::io::VTKDataSetReader source(fileName);
  return source.ReadDataSet();
}

std::string FileSceneBase::GetFieldName(const beams::Preset& vtkmNotUsed(preset))
{
  return "";
}

void FileSceneBase::Init(const beams::Preset& preset)
{
  auto mpi = pilot::mpi::Environment::Get();
  this->ShapeMpiTopology(mpi);
  this->DataSet = this->GetDataSet(preset);
  this->FieldName = this->GetFieldName(preset);
  this->Range = vtkm::Range{ 0.0f, 255.0f };
  this->Id = preset.Id;

  this->BoundsMap = std::make_shared<beams::rendering::BoundsMap>(this->DataSet);

  bool presetHasColorTable = preset.ColorTableOptions.Name.size() != 0;
  if (presetHasColorTable)
  {
    LOG::Println0("Using preset colorTable");
    vtkm::cont::ColorTable colorTable(preset.ColorTableOptions.Name);
    for (const auto& pointAlpha : preset.ColorTableOptions.PointAlphas)
    {
      LOG::Println0("x = {}, alpha = {}", pointAlpha.first, pointAlpha.second);
      colorTable.AddPointAlpha(pointAlpha.first, pointAlpha.second);
    }
    this->ColorTable = colorTable;
  }
  else
  {
    vtkm::cont::ColorTable colorTable; //{ "cool to warm" };
    colorTable.AddPointAlpha(0.0, 0.0f);
    colorTable.AddPointAlpha(0.199, 0.0f);
    colorTable.AddPointAlpha(0.2, 0.1f);
    colorTable.AddPointAlpha(1.0, 1.0f);

    this->ColorTable = colorTable;
  }
  this->Azimuth = preset.CameraOptions.Azimuth;
  this->Elevation = preset.CameraOptions.Elevation;

  this->LightPosition = preset.LightOptions.Lights[0].Position;
  this->LightColor = preset.LightOptions.Lights[0].Color;
  this->ShadowMapSize = { 64, 64, 64 };
  // this->ShadowMapSize = { 32 };
  using FloatHandle = vtkm::cont::ArrayHandle<vtkm::FloatDefault>;
  using RectilinearPoints =
    vtkm::cont::ArrayHandleCartesianProduct<FloatHandle, FloatHandle, FloatHandle>;
  auto coords = this->DataSet.GetCoordinateSystem().GetData().AsArrayHandle<RectilinearPoints>();
  vtkm::Id3 dims{ coords.GetFirstArray().GetNumberOfValues(),
                  coords.GetSecondArray().GetNumberOfValues(),
                  coords.GetThirdArray().GetNumberOfValues() };

  LOG::Println0("DataSet size = {}", dims);
  LOG::Println0("Opacity map size = {}", this->ShadowMapSize);

  auto& blockBounds = this->BoundsMap->BlockBounds;
  for (vtkm::Id i = 0; i < mpi->Size; ++i)
  {
    LOG::Println0("Local: Bounds {} => {}", i, blockBounds[i]);
  }
}

beams::Result FileSceneBase::Ready()
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
  // this->Mapper.SetSampleDistance(0.001f);
  this->Mapper.SetDensityScale(1.0f);
  this->Mapper.SetBoundsMap(this->BoundsMap.get());
  this->Mapper.SetUseShadowMap(true);
  this->Mapper.SetShadowMapSize(this->ShadowMapSize);
  std::cerr << "\033[1;31m" << this->LightPosition << "\033[0m\n";
  std::shared_ptr<beams::rendering::Light> light =
    std::make_shared<beams::rendering::PointLight<vtkm::Float32>>(
      this->LightPosition, this->LightColor, 2.0f);
  this->Mapper.ClearLights();
  this->Mapper.AddLight(light);

  return beams::Result::Succeeded();
}
}
} //namespace beams::rendering
