#pragma once

#include <pilot/staging/DescriptorResult.h>
#include <pilot/staging/TOMLDescriptor.h>

#include <toml++/toml.h>

#include <string>

namespace pilot
{
namespace staging
{
struct NamedDescriptor : public TOMLDescriptor
{
  std::string Name;

  NamedDescriptor() = default;
  virtual ~NamedDescriptor() = default;

  virtual DescriptorResult Deserialize(const toml::table& descriptorTable) override;

  virtual DescriptorResult Deserialize(const toml::key& name, const toml::table& descriptorTable);
};
}
}