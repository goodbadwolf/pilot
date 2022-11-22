#pragma once

#include <cstdlib>
#include <iostream>
#include <string>

namespace pilot
{
namespace system
{
void Fail(const std::string& message)
{
  std::cerr << "Failure: " << message << "\n";
  exit(EXIT_FAILURE);
}
}
}