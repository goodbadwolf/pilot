#pragma once

#include <pilot/Result.h>

#include <string>

namespace pilot
{
namespace staging
{
using DescriptorResult = pilot::Result<bool, std::string>;
}
}