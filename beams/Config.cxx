#include "Config.h"

#include "fmt/core.h"

#include <fstream>
#include <sstream>

namespace beams
{
template <typename Type>
beams::Result DeserializeToType(const PJObj& obj, const std::string& attrName, Type& expected)
{
  if (obj.find(attrName) == obj.end())
  {
    return Result::Failed(fmt::format("No attribute {}", attrName));
  }
  const PJVal& val = obj.at(attrName);
  if (!val.is<PJObj>())
  {
    return Result::Failed(fmt::format("Invalid {} attribute structure", attrName));
  }
  const PJObj& expectedObj = val.get<PJObj>();
  auto result = expected.Deserialize(expectedObj);
  if (!result.Success)
  {
    return Result::Failed(fmt::format("{}", result.Err));
  }

  return Result::Succeeded();
}

template <typename NativeType, typename JsonType = NativeType>
beams::Result DeserializeToNativeType(const PJObj& obj,
                                      const std::string& attrName,
                                      NativeType& nativeVal)
{
  if (obj.find(attrName) == obj.end())
  {
    return Result::Failed(fmt::format("No attribute {}", attrName));
  }
  const PJVal& val = obj.at(attrName);
  if (!val.is<JsonType>())
  {
    return Result::Failed(fmt::format("Unsupported value type for {} attribute", attrName));
  }
  JsonType jsonVal = val.get<JsonType>();
  nativeVal = static_cast<NativeType>(jsonVal);
  return Result::Succeeded();
}

template <typename MapType>
beams::Result DeserializeToMap(const PJObj& obj, const std::string& attrName, MapType& map)
{
  if (obj.find(attrName) == obj.end())
  {
    return Result::Failed(fmt::format("No attribute {}", attrName));
  }
  const PJVal& val = obj.at(attrName);
  if (!val.is<PJObj>())
  {
    return Result::Failed(fmt::format("Unsupported value type for {} attribute", attrName));
  }
  for (auto const& entry : val.get<PJObj>())
  {
    const PJVal& tmpVal = entry.second;
    if (!tmpVal.is<std::string>())
      continue;
    map[entry.first] = entry.second.get<std::string>();
  }

  return Result::Succeeded();
}

template <typename Type>
beams::Result DeserializeToVector(const PJObj& obj,
                                  const std::string& attrName,
                                  std::vector<Type>& expected)
{
  if (obj.find(attrName) == obj.end())
  {
    return Result::Failed(fmt::format("No attribute {}", attrName));
  }
  const PJVal& val = obj.at(attrName);
  if (!val.is<PJArr>())
  {
    return Result::Failed(fmt::format("Invalid {} attribute structure", attrName));
  }
  const PJArr& valArr = val.get<PJArr>();
  for (PJArr::const_iterator i = valArr.cbegin(); i != valArr.cend(); ++i)
  {
    const PJVal& tmpVal = *i;
    if (!tmpVal.is<PJObj>())
    {
      return Result::Failed(fmt::format("Invalid structure inside {}", attrName));
    }
    const PJObj& tmpObj = tmpVal.get<PJObj>();
    Type e;
    auto result = e.Deserialize(tmpObj);
    if (!result.Success)
    {
      return Result::Failed(fmt::format("{}", result.Err));
    }
    expected.push_back(e);
  }
  return Result::Succeeded();
}

template <typename NativeType, typename JsonType = NativeType>
beams::Result DeserializeToVectorNative(const PJObj& obj,
                                        const std::string& attrName,
                                        std::vector<NativeType>& expected)
{
  if (obj.find(attrName) == obj.end())
  {
    return Result::Failed(fmt::format("No attribute {}", attrName));
  }
  const PJVal& val = obj.at(attrName);
  if (!val.is<PJArr>())
  {
    return Result::Failed(fmt::format("Invalid {} attribute structure", attrName));
  }
  const PJArr& valArr = val.get<PJArr>();
  for (PJArr::const_iterator i = valArr.cbegin(); i != valArr.cend(); ++i)
  {
    const PJVal& tmpVal = *i;
    if (!tmpVal.is<JsonType>())
    {
      return Result::Failed(fmt::format("Invalid structure inside {}", attrName));
    }
    expected.push_back(static_cast<NativeType>(tmpVal.get<JsonType>()));
  }
  return Result::Succeeded();
}

beams::Result DataSetOptions::Deserialize(const PJObj& optionsObj)
{
  CHECK_RESULT(DeserializeToNativeType(optionsObj, "factory", this->Factory),
               "Error reading dataSetOptions");
  CHECK_RESULT(DeserializeToMap(optionsObj, "params", this->Params),
               "Error reading dataSetOptions");
  return Result::Succeeded();
}

beams::Result OpacityMapOptions::Deserialize(const PJObj& optionsObj)
{
  const auto DeserializeToIdComponent = DeserializeToNativeType<vtkm::IdComponent, int64_t>;
  const auto DeserializeToFloat32 = DeserializeToNativeType<vtkm::Float32, double>;
  const auto DeserializeToId3 = DeserializeToVectorNative<vtkm::Id, double>;

  this->Size = { -1, -1, -1 };
  this->SizeRatio = 0.0f;
  CHECK_RESULT(DeserializeToNativeType(optionsObj, "enabled", this->Enabled),
               "Error reading opacityMapOptions");
  if (optionsObj.find("size") != optionsObj.end())
  {
    std::vector<vtkm::Id> tmpSize;
    CHECK_RESULT(DeserializeToId3(optionsObj, "size", tmpSize), "Error reading opacityMapOptions");
    this->Size = { tmpSize[0], tmpSize[1], tmpSize[2] };
  }
  else if (optionsObj.find("sizeRatio") != optionsObj.end())
  {
    CHECK_RESULT(DeserializeToFloat32(optionsObj, "sizeRatio", this->SizeRatio),
                 "Error reading opacityMapOptions");
  }
  else
  {
    return Result::Failed("Error reading opacityMapOptions: No size or sizeRatio");
  }
  CHECK_RESULT(DeserializeToIdComponent(optionsObj, "numSteps", this->NumSteps),
               "Error reading opacityMapOptions");
  return Result::Succeeded();
}

beams::Result CameraOptions::Deserialize(const PJObj& optionsObj)
{
  const auto DeserializeToFloat32 = DeserializeToNativeType<vtkm::Float32, double>;
  CHECK_RESULT(DeserializeToNativeType(optionsObj, "type", this->Type),
               "Error reading cameraOptions");
  CHECK_RESULT(DeserializeToFloat32(optionsObj, "azimuth", this->Azimuth),
               "Error reading cameraOptions");
  CHECK_RESULT(DeserializeToFloat32(optionsObj, "elevation", this->Elevation),
               "Error reading cameraOptions");
  return Result::Succeeded();
}

beams::Result LightOptions::Deserialize(const PJObj& optionsObj)
{
  CHECK_RESULT(DeserializeToVector(optionsObj, "lights", this->Lights), "Error reading lights");
  return Result::Succeeded();
}

beams::Result LightOption::Deserialize(const PJObj& optionsObj)
{
  const auto DeserializeToFloat32 = DeserializeToNativeType<vtkm::Float32, double>;
  const auto DeserializeToVecFloat32 = DeserializeToVectorNative<vtkm::Float32, double>;
  CHECK_RESULT(DeserializeToNativeType(optionsObj, "type", this->Type),
               "Error reading lightOption");
  std::vector<vtkm::Float32> tmpPos;
  std::vector<vtkm::Float32> tmpColor;
  CHECK_RESULT(DeserializeToVecFloat32(optionsObj, "position", tmpPos),
               "Error reading lightOption");
  this->Position = { tmpPos[0], tmpPos[1], tmpPos[2] };
  CHECK_RESULT(DeserializeToVecFloat32(optionsObj, "color", tmpColor), "Error reading lightOption");
  this->Color = { tmpColor[0], tmpColor[1], tmpColor[2] };
  CHECK_RESULT(DeserializeToFloat32(optionsObj, "intensity", this->Intensity),
               "Error reading lightOption");
  return Result::Succeeded();
}

beams::Result Preset::Deserialize(const PJObj& presetObj)
{
  CHECK_RESULT(DeserializeToNativeType(presetObj, "id", this->Id), "Error reading preset");
  CHECK_RESULT(DeserializeToType(presetObj, "dataSetOptions", this->DataSetOptions),
               fmt::format("Error reading preset '{}'", this->Id));
  CHECK_RESULT(DeserializeToType(presetObj, "opacityMapOptions", this->OpacityMapOptions),
               fmt::format("Error reading preset '{}'", this->Id));
  CHECK_RESULT(DeserializeToType(presetObj, "cameraOptions", this->CameraOptions),
               fmt::format("Error reading preset '{}'", this->Id));
  CHECK_RESULT(DeserializeToType(presetObj, "lightOptions", this->LightOptions),
               fmt::format("Error reading preset '{}'", this->Id));

  return Result::Succeeded();
}

beams::Result Config::LoadFromFile(const std::string& filePath)
{
  std::ifstream jsonFile(filePath);

  PJVal jsonVal;
  std::string err = picojson::parse(jsonVal, jsonFile);
  if (!err.empty())
  {
    return Result::Failed(fmt::format("Error parsing config: {}", err));
  }
  if (!jsonVal.is<PJObj>())
  {
    return Result::Failed(fmt::format("Error reading config: Invalid structure"));
  }
  const PJObj& jsonRoot = jsonVal.get<PJObj>();

  {
    if (jsonRoot.find("presets") == jsonRoot.end())
    {
      return Result::Failed(fmt::format("Error reading config: No 'presets' key"));
    }
    const PJVal& presetsVal = jsonRoot.at("presets");
    if (!presetsVal.is<PJArr>())
    {
      return Result::Failed(fmt::format("Error reading config: 'presets' should be array"));
    }
    const PJArr& presetsArr = presetsVal.get<PJArr>();
    if (presetsArr.size() == 0)
    {
      return Result::Failed(fmt::format("Error reading config: 'presets' is empty array"));
    }

    for (PJArr::const_iterator i = presetsArr.begin(); i != presetsArr.end(); ++i)
    {
      const PJVal& presetVal = *i;
      if (!presetVal.is<PJObj>())
      {
        return Result::Failed(fmt::format("Error reading config: Invalid preset structure"));
      }

      const PJObj& presetObj = presetVal.get<PJObj>();
      Preset preset;
      auto result = preset.Deserialize(presetObj);
      if (!result.Success)
      {
        return Result::Failed(fmt::format("Error reading config: {}", result.Err));
      }
      this->Presets[preset.Id] = preset;
    }
  }

  {
    if (jsonRoot.find("defaultPreset") == jsonRoot.end())
    {
      return Result::Failed("Error reading config: No 'defaultPreset' key");
    }
    const PJVal& defaultPresetVal = jsonRoot.at("defaultPreset");
    if (!defaultPresetVal.is<std::string>())
    {
      return Result::Failed("Error reading config: 'defaultPreset' must be string");
    }
    std::string defaultPreset = defaultPresetVal.get<std::string>();
    if (this->Presets.find(defaultPreset) == this->Presets.end())
    {
      return Result::Failed(
        fmt::format("Error reading config: No preset with id '{}'", defaultPreset));
    }
    this->DefaultPresetName = defaultPreset;
  }

  return Result::Succeeded();
}

std::ostream& operator<<(std::ostream& os, const Preset& preset)
{
  os << "Preset:\n Id = " << preset.Id << "\n DataSetOptions = " << preset.DataSetOptions
     << "\n OpacityMapOptions = " << preset.OpacityMapOptions
     << "\n CameraOptions = " << preset.CameraOptions << "\n LightOptions = " << preset.LightOptions
     << "\n";
  return os;
}

std::ostream& operator<<(std::ostream& os, const DataSetOptions& options)
{
  std::stringstream ss;
  for (auto const& entry : options.Params)
  {
    ss << entry.first << " => " << entry.second << ", ";
  }
  os << "Factory = " << options.Factory << ", Params = {" << ss.str() << "}";
  return os;
}

std::ostream& operator<<(std::ostream& os, const OpacityMapOptions& options)
{
  os << std::boolalpha;
  os << "Enabled = " << options.Enabled << ", Size = " << options.Size
     << ", NumSteps = " << options.NumSteps;
  os << std::noboolalpha;
  return os;
}

std::ostream& operator<<(std::ostream& os, const CameraOptions& options)
{
  os << "Type = " << options.Type << ", Azimuth = " << options.Azimuth
     << ", Elevation = " << options.Elevation;
  return os;
}

std::ostream& operator<<(std::ostream& os, const LightOptions& options)
{
  std::stringstream ss;
  for (auto const& light : options.Lights)
  {
    ss << "{" << light << "}, ";
  }
  os << "Lights = [" << ss.str() << "]";
  return os;
}

std::ostream& operator<<(std::ostream& os, const LightOption& option)
{
  os << "Type = " << option.Type << ", Position = " << option.Position
     << ", Color = " << option.Color << ", Intensity = " << option.Intensity;
  return os;
}

} // namespace beams
