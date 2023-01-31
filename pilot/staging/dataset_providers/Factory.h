#pragma once

#include <pilot/staging/dataset_providers/Descriptor.h>

#include <toml++/toml.h>

#include <memory>

namespace pilot
{
namespace staging
{
namespace dataset_providers
{
using CreateResult = pilot::Result<DescriptorPtr, std::string>;

struct Factory
{
  static CreateResult CreateFromToml(const toml::key& name, const toml::table& descriptorTable);
};
}
}
}