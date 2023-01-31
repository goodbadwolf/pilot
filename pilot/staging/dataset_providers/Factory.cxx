#include <pilot/Result.h>
#include <pilot/TomlUtils.h>
#include <pilot/staging/dataset_providers/Factory.h>
#include <pilot/staging/dataset_providers/SubdividedSpheresDescriptor.h>

#include <fmt/core.h>

namespace pilot
{
namespace staging
{
namespace dataset_providers
{

namespace
{
DescriptorPtr CreateFromType(const std::string& type)
{
  DescriptorPtr descriptor = nullptr;
  if (type == "subdivided_spheres")
  {
    descriptor = std::make_shared<pilot::staging::dataset_providers::SubdividedSpheresDescriptor>();
  }
  return descriptor;
}
}

CreateResult Factory::CreateFromToml(const toml::key& name, const toml::table& descriptorTable)
{
  using Toml = pilot::TomlUtils;

  std::string type;
  auto typeResult = Toml::Deserialize<std::string>(descriptorTable, "type");
  if (!typeResult.IsSuccess())
  {
    return CreateResult::Fail(
      fmt::format("Failed to read type for DataSetProvider: \n{}\n", descriptorTable));
  }
  type = typeResult.Value;

  DescriptorPtr descriptor = CreateFromType(type);
  if (!descriptor)
  {
    return CreateResult::Fail(fmt::format("Failed to create DataSetProvider for type: '{}'", type));
  }

  descriptor->Deserialize(name, descriptorTable);
  return CreateResult::Success(descriptor);
}
}
}
}