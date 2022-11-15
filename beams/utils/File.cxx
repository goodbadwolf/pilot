#include "File.h"

#include <vtkm/io/VTKDataSetWriter.h>

#include <fstream>

namespace beams
{
namespace io
{

bool File::FileExists(const std::string& filePath)
{
  std::ifstream file(filePath);
  return file.is_open();
}

void File::SaveDataSet(const vtkm::cont::DataSet& dataSet, const std::string& name)
{
  vtkm::io::VTKDataSetWriter writer(name);
  writer.WriteDataSet(dataSet);
}

}
} // namespace beams::io