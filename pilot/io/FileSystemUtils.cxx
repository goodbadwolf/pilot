#include <pilot/io/FileSystemUtils.h>

#include <fstream>

namespace pilot
{
namespace io
{
bool FileSystemUtils::FileExists(const std::string& path)
{
  std::ifstream file(path);
  return file.is_open();
}
}
}