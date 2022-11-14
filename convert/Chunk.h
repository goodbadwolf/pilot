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
using DataSets = std::vector<vtkm::cont::DataSet>;
using ChunkResult = pilot::Result<DataSets, std::string>;
using FilterResult = pilot::Result<vtkm::cont::DataSet, std::string>;
using SaveResult = pilot::Result<bool, std::string>;
using UniformPoints = vtkm::cont::ArrayHandleUniformPointCoordinates;
using FloatHandle = vtkm::cont::ArrayHandle<vtkm::FloatDefault>;
using RectilinearPoints =
  vtkm::cont::ArrayHandleCartesianProduct<FloatHandle, FloatHandle, FloatHandle>;

ChunkResult ChunkDataSet(const vtkm::cont::DataSet& dataSet,
                         int chunksX,
                         int chunksY,
                         int chunksZ,
                         vtkm::filter::FieldSelection fields);

SaveResult SaveChunksToDisk(const DataSets& dataSets,
                            const std::string& fileNamePrefix,
                            vtkm::io::FileType fileType = vtkm::io::FileType::ASCII);

FilterResult FilterFields(vtkm::cont::DataSet& dataSet,
                          vtkm::filter::FieldSelection fieldSelection);
}
}
}
