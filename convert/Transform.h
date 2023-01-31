#pragma once

#include <pilot/Result.h>

#include <vtkm/cont/DataSet.h>
#include <vtkm/filter/FieldSelection.h>
#include <vtkm/io/VTKDataSetWriter.h>

#include <string>
#include <vector>

namespace pilot
{
namespace apps
{
namespace convert
{
using TransformResult = pilot::Result<vtkm::cont::DataSet, std::string>;

TransformResult TransformDataSet(const vtkm::cont::DataSet& dataSet,
                                 const vtkm::Vec3f_32& translation,
                                 vtkm::filter::FieldSelection fields);
}
}
}
