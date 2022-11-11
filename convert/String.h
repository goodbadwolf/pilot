#pragma once

#include <sstream>
#include <string>

bool ReplaceFirstInline(std::string& input, const std::string& what, const std::string& with)
{
  std::size_t pos = input.find(what);
  if (pos == std::string::npos)
  {
    return false;
  }
  input.replace(pos, what.length(), with);
  return true;
}

std::string ReplaceAll(const std::string& input, const std::string& what, const std::string& with)
{
  std::string result(input);
  while (ReplaceFirstInline(result, what, with))
    ;
  return result;
}

template <typename Range, typename Value = typename Range::value_type>
std::string Join(const Range& elements, const char* const delimiter)
{
  std::ostringstream os;
  auto b = std::begin(elements), e = std::end(elements);

  if (b != e)
  {
    std::copy(b, std::prev(e), std::ostream_iterator<Value>(os, delimiter));
    b = std::prev(e);
  }
  if (b != e)
  {
    os << *b;
  }

  return os.str();
}