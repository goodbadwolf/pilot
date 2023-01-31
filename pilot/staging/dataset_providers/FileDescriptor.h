#pragma once

#include <pilot/staging/dataset_providers/Descriptor.h>

#include <vtkm/Types.h>

namespace pilot
{
namespace staging
{
namespace dataset_providers
{
struct FileDescriptor : public Descriptor
{
  std::string FileNamePattern;

  virtual DescriptorResult Deserialize(const toml::key& name,
                                       const toml::table& descriptorTable) override;
};
}
}
}