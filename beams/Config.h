#ifndef beams_config_h
#define beams_config_h

#include "Result.h"

#include <pilot/Result.h>
#include <pilot/staging/DescriptorResult.h>
#include <pilot/staging/Stage.h>

#include <vtkm/Types.h>

#include <string>
#include <unordered_map>

namespace beams
{
/*
struct DataSetOptions
{
  beams::Result Deserialize(const PJObj& optionsObj);

  friend std::ostream& operator<<(std::ostream& os, const DataSetOptions& preset);

  std::string Factory;
  std::unordered_map<std::string, std::string> Params;
};

struct OpacityMapOptions
{
  beams::Result Deserialize(const PJObj& optionsObj);

  vtkm::Id3 CalculateSize(vtkm::Id3 dataSetSize) const
  {
    bool shouldCalculateSize = this->Size[0] == -1;
    if (shouldCalculateSize)
    {
      return vtkm::Id3{ static_cast<vtkm::Id>(dataSetSize[0] * this->SizeRatio),
                        static_cast<vtkm::Id>(dataSetSize[1] * this->SizeRatio),
                        static_cast<vtkm::Id>(dataSetSize[2] * this->SizeRatio) };
    }
    else
    {
      return this->Size;
    }
  }

  friend std::ostream& operator<<(std::ostream& os, const OpacityMapOptions& preset);

  bool Enabled;
  vtkm::Id3 Size;
  vtkm::Float32 SizeRatio;
  vtkm::IdComponent NumSteps;
};

struct CameraOptions
{
  beams::Result Deserialize(const PJObj& optionsObj);

  friend std::ostream& operator<<(std::ostream& os, const CameraOptions& preset);

  std::string Type;
  vtkm::Float32 Azimuth;
  vtkm::Float32 Elevation;
};

struct LightOption
{
  beams::Result Deserialize(const PJObj& optionsObj);

  friend std::ostream& operator<<(std::ostream& os, const LightOption& preset);

  std::string Type;
  vtkm::Vec3f_32 Position;
  vtkm::Vec3f_32 Color;
  vtkm::Float32 Intensity;
};

struct LightOptions
{
  beams::Result Deserialize(const PJObj& optionsObj);

  friend std::ostream& operator<<(std::ostream& os, const LightOptions& preset);

  std::vector<LightOption> Lights;
};

struct ColorTableOptions
{
  beams::Result Deserialize(const PJObj& optionsObj);

  friend std::ostream& operator<<(std::ostream& os, const ColorTableOptions& options);

  std::string Name;
  vtkm::Float32 RangeMin;
  vtkm::Float32 RangeMax;
  std::vector<std::pair<vtkm::Float64, vtkm::Float32>> PointAlphas;
};
*/

struct Preset
{
  /*
  beams::Result Deserialize(const PJObj& presetObj);

  friend std::ostream& operator<<(std::ostream& os, const Preset& preset);

  std::string Id;
  beams::DataSetOptions DataSetOptions;
  beams::OpacityMapOptions OpacityMapOptions;
  beams::CameraOptions CameraOptions;
  beams::LightOptions LightOptions;
  beams::ColorTableOptions ColorTableOptions;
  */
}; // struct Preset

struct Config : public pilot::staging::Stage
{
  Config() = default;
  virtual ~Config() = default;

  virtual pilot::staging::DescriptorResult Deserialize(const toml::table& descriptorTable) override;

  //std::string DefaultPresetName;
  //std::map<std::string, Preset> Presets;
}; // struct Config

} // namespace beams


#endif // beams_config_h