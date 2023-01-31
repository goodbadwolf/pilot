#pragma once

#include <pilot/staging/DescriptorResult.h>

#include <toml++/toml.h>

namespace pilot
{
namespace staging
{
struct TOMLDescriptor
{
  TOMLDescriptor() = default;

  virtual ~TOMLDescriptor() = default;

  virtual DescriptorResult Deserialize(const toml::table& descriptorTable) = 0;
};
}
}