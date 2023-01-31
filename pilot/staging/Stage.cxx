#include <pilot/Result.h>
#include <pilot/TomlUtils.h>
#include <pilot/staging/Stage.h>
#include <pilot/staging/dataset_providers/Factory.h>

namespace pilot
{
namespace staging
{
#define CHECK_RESULT(statement)                    \
  {                                                \
    auto result = (statement);                     \
    if (!result.IsSuccess())                       \
    {                                              \
      return DescriptorResult::Fail(result.Error); \
    }                                              \
  }

DescriptorResult Stage::Deserialize(const toml::table& descriptorTable)
{
  CHECK_RESULT(this->DeserializeDataSetProviders(descriptorTable));
  return DescriptorResult::Success(true);
}

DescriptorResult Stage::DeserializeDataSetProviders(const toml::table& descriptorTable)
{
  auto dataSetProvidersView = descriptorTable.at_path("dataset_providers");
  if (!dataSetProvidersView)
  {
    return DescriptorResult::Fail("Failed to find dataset_providers");
  }

  const auto* dataSetProvidersTable = dataSetProvidersView.as_table();
  for (auto i = dataSetProvidersTable->cbegin(); i != dataSetProvidersTable->cend(); ++i)
  {
    const auto& name = i->first;
    const auto* dataSetProviderTable = i->second.as_table();
    auto result = dataset_providers::Factory::CreateFromToml(name, *dataSetProviderTable);
    if (result.IsFailure())
    {
      fmt::print("Skipping creating failed creation of DataSetProvider: {}\n", result.Error);
      continue;
    }
    auto provider = result.Value;
    this->DataSetProviderDescriptors.insert({ provider->Name, provider });
  }

  return DescriptorResult::Success(true);
}

#undef CHECK_RESULT
}
}