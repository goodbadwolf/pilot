#include <pilot/staging/dataset_providers/Descriptor.h>

namespace pilot
{
namespace staging
{
namespace dataset_providers
{
DescriptorResult Descriptor::Deserialize(const toml::key& name, const toml::table& descriptorTbl)
{
  return pilot::staging::NamedDescriptor::Deserialize(name, descriptorTbl);
}
}
}
}
