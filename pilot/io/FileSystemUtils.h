#pragma once

#include <string>

namespace pilot
{
namespace io
{
struct FileSystemUtils
{
  static bool FileExists(const std::string& path);
};
}
}