#pragma once

#include <sstream>
#include <string>

namespace pilot
{
struct StringUtils
{
  inline static bool ReplaceFirstInline(std::string& input,
                                        const std::string& what,
                                        const std::string& with)
  {
    std::size_t pos = input.find(what);
    if (pos == std::string::npos)
    {
      return false;
    }
    input.replace(pos, what.length(), with);
    return true;
  }

  inline static std::string ReplaceAll(const std::string& input,
                                       const std::string& what,
                                       const std::string& with)
  {
    std::string result(input);
    while (StringUtils::ReplaceFirstInline(result, what, with))
      ;
    return result;
  }

  template <typename Range>
  inline static std::string Join(const Range& elements, const std::string& delimiter)
  {
    std::ostringstream os;
    auto begin = std::begin(elements), end = std::end(elements);

    if (begin != end)
    {
      std::copy(begin,
                std::prev(end),
                std::ostream_iterator<typename Range::value_type>(os, delimiter.c_str()));
      begin = std::prev(end);
    }
    if (begin != end)
    {
      os << *begin;
    }

    return os.str();
  }
};
}