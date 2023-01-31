#pragma once

#include <pilot/staging/NamedDescriptor.h>

#include <memory>
#include <string>

namespace pilot
{
namespace staging
{
namespace dataset_providers
{
struct Descriptor : public pilot::staging::NamedDescriptor
{
  Descriptor() = default;

  virtual ~Descriptor() = default;

  virtual pilot::staging::DescriptorResult Deserialize(const toml::key& name,
                                                       const toml::table& descriptorTable) override;

  std::string Type;
};

using DescriptorPtr = std::shared_ptr<pilot::staging::dataset_providers::Descriptor>;
}
}
}