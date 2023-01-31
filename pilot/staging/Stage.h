#pragma once

#include <pilot/staging/TOMLDescriptor.h>
#include <pilot/staging/dataset_providers/Descriptor.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace pilot
{
namespace staging
{
struct Stage : public TOMLDescriptor
{
  std::unordered_map<std::string, std::shared_ptr<pilot::staging::dataset_providers::Descriptor>>
    DataSetProviderDescriptors;

  Stage() = default;
  virtual ~Stage() = default;

  virtual DescriptorResult Deserialize(const toml::table& descriptorTable) override;

protected:
  virtual DescriptorResult DeserializeDataSetProviders(const toml::table& descriptorTable);
};
}
}
