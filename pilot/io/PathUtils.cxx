#include <pilot/io/PathUtils.h>

#include <fstream>

namespace pilot
{
namespace io
{
bool PathUtils::FileExists(const std::string& path)
{
  std::ifstream file(path);
  return file.is_open();
}
}
}