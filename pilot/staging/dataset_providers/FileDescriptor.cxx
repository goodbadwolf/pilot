#include <pilot/TomlUtils.h>
#include <pilot/staging/dataset_providers/FileDescriptor.h>

namespace pilot
{
namespace staging
{
namespace dataset_providers
{
#define CHECK_RESULT(statement, var)               \
  {                                                \
    auto result = (statement);                     \
    if (!result)                                   \
    {                                              \
      return DescriptorResult::Fail(result.Error); \
    }                                              \
    var = result.Value;                            \
  }

DescriptorResult FileDescriptor::Deserialize(const toml::key& name,
                                             const toml::table& descriptorTable)
{
  using Toml = pilot::TomlUtils;

  auto baseResult =
    pilot::staging::dataset_providers::Descriptor::Deserialize(name, descriptorTable);
  if (!baseResult)
  {
    return DescriptorResult::Fail(baseResult.Error);
  }

  const std::string FILE_NAME_PATTERN = "file_name_pattern";
  CHECK_RESULT(Toml::Deserialize<std::string>(descriptorTable, FILE_NAME_PATTERN),
               this->FileNamePattern);
  return DescriptorResult::Success(true);
}

#undef CHECK_RESULT
}
}
}