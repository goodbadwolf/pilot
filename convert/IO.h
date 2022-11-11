#pragma once

#include "Result.h"

#include <vtkm/io/VTKDataSetReader.h>
#include <vtkm/io/VTKDataSetWriter.h>

#include <string>

using ReadResult = Result<vtkm::cont::DataSet, std::string>;

using WriteResult = Result<bool, std::string>;

ReadResult ReadDataSet(const std::string& name)
{
  try
  {
    vtkm::io::VTKDataSetReader reader(name);
    auto dataSet = reader.ReadDataSet();
    return ReadResult(dataSet);
  }
  catch (const vtkm::cont::Error& e)
  {
    return ReadResult(e.GetMessage());
  }
}

WriteResult SaveDataSet(const vtkm::cont::DataSet& dataSet,
                        const std::string& fileName,
                        vtkm::io::FileType fileType = vtkm::io::FileType::ASCII)
{
  try
  {
    vtkm::io::VTKDataSetWriter writer(fileName);
    writer.SetFileType(fileType);
    writer.WriteDataSet(dataSet);
    return WriteResult(true);
  }
  catch (const vtkm::cont::Error& e)
  {
    return WriteResult(e.GetMessage());
  }
}