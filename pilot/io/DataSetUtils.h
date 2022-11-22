#pragma once

#include <pilot/Result.h>

#include <vtkm/cont/DataSet.h>
#include <vtkm/io/VTKDataSetWriter.h>

#include <string>

namespace pilot
{
namespace io
{
using ReadResult = pilot::Result<vtkm::cont::DataSet, std::string>;
using WriteResult = pilot::Result<bool, std::string>;

struct DataSetUtils
{
  static ReadResult Read(const std::string& name);

  static WriteResult Write(const vtkm::cont::DataSet& dataSet,
                           const std::string& fileName,
                           vtkm::io::FileType fileType = vtkm::io::FileType::ASCII);
};
}
}
