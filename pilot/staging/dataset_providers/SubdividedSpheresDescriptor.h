#pragma once

#include <pilot/staging/dataset_providers/Descriptor.h>

#include <vtkm/Types.h>

namespace pilot
{
namespace staging
{
namespace dataset_providers
{
struct SubdividedSpheresDescriptor : public Descriptor
{
  vtkm::Id3 CellDims;

  virtual DescriptorResult Deserialize(const toml::key& name,
                                       const toml::table& descriptorTable) override;
};
}
}
}