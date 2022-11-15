#ifndef beams_utils_file_h
#define beams_utils_file_h

#include <vtkm/cont/DataSet.h>

#include <string>

namespace beams
{
namespace io
{

struct File
{
  static bool FileExists(const std::string& filePath);

  static void SaveDataSet(const vtkm::cont::DataSet& dataSet, const std::string& name);
};

}
} // namespace beams::io

#endif // beams_utils_file_h