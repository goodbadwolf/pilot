#include <pilot/TomlUtils.h>
#include <pilot/staging/NamedDescriptor.h>

namespace pilot
{
namespace staging
{
DescriptorResult NamedDescriptor::Deserialize(const toml::table& descriptorTable)
{
  return this->Deserialize(toml::key("Unnamed"), descriptorTable);
}

DescriptorResult NamedDescriptor::Deserialize(const toml::key& name, const toml::table&)
{
  this->Name = name.str();
  return DescriptorResult::Success(true);
}
}
}