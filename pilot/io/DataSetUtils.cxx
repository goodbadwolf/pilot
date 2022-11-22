#include <pilot/Result.h>
#include <pilot/io/DataSetUtils.h>

#include <vtkm/io/VTKDataSetReader.h>
#include <vtkm/io/VTKDataSetWriter.h>

#include <string>

namespace pilot
{
namespace io
{
ReadResult DataSetUtils::Read(const std::string& name)
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

WriteResult DataSetUtils::Write(const vtkm::cont::DataSet& dataSet,
                                const std::string& fileName,
                                vtkm::io::FileType fileType)
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
}
}