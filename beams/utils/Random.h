#ifndef beams_utils_random_h
#define beams_utils_random_h

#include <random>

namespace beams
{
namespace utils
{
struct Random
{
  template <typename Precision>
  Precision Random01()
  {
    static std::mt19937 mt(0);
    static std::uniform_real_distribution<typename Precision> dist(0.0f, 1.0f);
    return dist(mt);
  }
};
} // namespace utils
} // namespace beams

#endif