#pragma once

#include <string>

namespace pilot
{
namespace io
{
struct PathUtils
{
  static bool FileExists(const std::string& path);
};
}
}