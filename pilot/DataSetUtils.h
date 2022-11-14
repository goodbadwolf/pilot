#pragma once

#include <pilot/Result.h>

#include <vtkm/io/VTKDataSetReader.h>
#include <vtkm/io/VTKDataSetWriter.h>

#include <string>

namespace pilot
{
struct DataSetUtils
{
  using ReadResult = pilot::Result<vtkm::cont::DataSet, std::string>;
  using WriteResult = pilot::Result<bool, std::string>;

  inline static ReadResult Read(const std::string& name)
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

  inline static WriteResult Write(const vtkm::cont::DataSet& dataSet,
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
};
}
