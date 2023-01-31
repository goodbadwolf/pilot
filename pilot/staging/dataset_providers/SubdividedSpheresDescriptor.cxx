#include <pilot/TomlUtils.h>
#include <pilot/staging/dataset_providers/SubdividedSpheresDescriptor.h>

namespace pilot
{
namespace staging
{
namespace dataset_providers
{
DescriptorResult SubdividedSpheresDescriptor::Deserialize(const toml::key& name,
                                                          const toml::table& descriptorTable)
{
  using Toml = pilot::TomlUtils;

  auto baseResult =
    pilot::staging::dataset_providers::Descriptor::Deserialize(name, descriptorTable);
  if (!baseResult)
  {
    return DescriptorResult::Fail(baseResult.Error);
  }

  const std::string CELL_DIMS = "cell_dims";
  if (Toml::IsInteger(descriptorTable, CELL_DIMS))
  {
    auto cellDimsResult = Toml::Deserialize<int64_t>(descriptorTable, CELL_DIMS);
    if (!cellDimsResult)
    {
      return DescriptorResult::Fail(
        fmt::format("Failed to parse {}: {}", CELL_DIMS, cellDimsResult.Error));
    }
    int64_t cellDim = cellDimsResult.Value;
    this->CellDims = vtkm::Id3{ cellDim };
  }
  else if (Toml::IsArrayOfIntegers(descriptorTable, CELL_DIMS))
  {
    auto cellDimsResult = Toml::DeserializeArray<int64_t, 3>(descriptorTable, CELL_DIMS);
    if (!cellDimsResult)
    {
      return DescriptorResult::Fail(
        fmt::format("Failed to parse {}: {}", CELL_DIMS, cellDimsResult.Error));
    }
    std::array<int64_t, 3> cellDims = cellDimsResult.Value;
    this->CellDims = vtkm::Id3{ cellDims[0], cellDims[1], cellDims[2] };
  }

  return DescriptorResult::Success(true);
}
}
}
}