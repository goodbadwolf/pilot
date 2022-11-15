#include "DataSetFactory.h"
#include "SphereSource.h"

#include "Json.h"

#include <fmt/core.h>

namespace
{

beams::Result GenerateSphereDataSet(const beams::Preset& preset,
                                      vtkm::cont::DataSet& dataSet,
                                      std::string& fieldName)
{
  const vtkm::Vec3f defaultOrigin{ 0.0f, 0.0f, 0.0f };
  const vtkm::Id defaultDim = 128;
  const vtkm::FloatDefault defaultFieldValue = 0.12f;

  auto& sourceData = preset.DataSet.Data;

  vtkm::Id3 dims{ defaultDim, defaultDim, defaultDim };
  vtkm::Vec3f origin = defaultOrigin;

  if (sourceData.find("dims") != sourceData.end())
  {
    const PJArr& dimsArr = sourceData.at("dims").get<PJArr>();
    dims[0] = static_cast<vtkm::Id>(dimsArr.at(0).get<int64_t>());
    dims[1] = static_cast<vtkm::Id>(dimsArr.at(1).get<int64_t>());
    dims[2] = static_cast<vtkm::Id>(dimsArr.at(2).get<int64_t>());
  }

  if (sourceData.find("origin") != sourceData.end())
  {
    const PJArr& originArr = sourceData.at("origin").get<PJArr>();
    // picojson only supports deserializing from double and not float
    origin[0] = static_cast<vtkm::FloatDefault>(originArr.at(0).get<vtkm::Float64>());
    origin[1] = static_cast<vtkm::FloatDefault>(originArr.at(1).get<vtkm::Float64>());
    origin[2] = static_cast<vtkm::FloatDefault>(originArr.at(2).get<vtkm::Float64>());
  }

  fmt::print("Using sphere source dims: ({}, {}, {})\n", dims[0], dims[1], dims[2]);
  fmt::print("Using sphere source origin: ({}, {}, {})\n", origin[0], origin[1], origin[2]);

  beams::Spheres source(origin, dims, defaultFieldValue);
  source.AddSphere({ 0.5f, 0.5f, 0.5f }, 0.25f, 0.8f, 0.8f);   // Sphere at center
  source.AddSphere({ 0.1f, 0.25f, 0.5f }, 0.1f, 0.8f, 0.8f);   // Small sphere at the left
  source.AddSphere({ 0.5f, -99.9f, 0.5f }, 100.f, 0.8f, 0.8f); // Floor

  dataSet = source.Execute();
  fieldName = "sphere";

  return beams::Result::Succeeded();
}

beams::Result GenerateDataSet(const beams::Preset& preset,
                                vtkm::cont::DataSet& dataSet,
                                std::string& fieldName)
{
  auto factoryName = preset.DataSet.Factory;
  if (factoryName == "SphereDataSetFactory")
  {
    return GenerateSphereDataSet(preset, dataSet, fieldName);
  }

  return beams::Result::Failed(
    fmt::format("Unknown dataSet factory '{}' for preset '{}'", factoryName, preset.Id));
}

}

namespace beams
{

beams::Result DataSetFactory::CreateDataSetFromPreset(const beams::Preset& preset,
                                                        vtkm::cont::DataSet& dataSet,
                                                        std::string& fieldName)
{
  beams::Result result;
  result.Success = true;

  switch (preset.DataSet.Type)
  {
    case PresetDataSet::DataSetType::RuntimeGenerated:
    {
      result = GenerateDataSet(preset, dataSet, fieldName);
      break;
    }
    default:
    {
      result.Err = fmt::format("Unable to create dataset for preset: {}", preset.Id);
      break;
    }
  }

  return result;
}

}