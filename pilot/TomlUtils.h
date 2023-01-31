#pragma once

#include <pilot/Result.h>

#include <fmt/core.h>
#include <toml++/toml.h>

#include <sstream>
#include <string>

template <>
struct fmt::formatter<toml::table>
{
  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin())
  {
    auto it = ctx.begin();
    ++it;
    return it;
  }

  template <typename FormatContext>
  auto format(const toml::table& tbl, FormatContext& ctx) const -> decltype(ctx.out())
  {
    std::stringstream ss;
    ss << tbl;
    return fmt::format_to(ctx.out(), "{}", ss.str());
  }
};


namespace pilot
{
struct TomlUtils
{
  template <typename T>
  static Result<T, std::string> Deserialize(const toml::table& table, const std::string& key)
  {
    using TResult = Result<T, std::string>;

    bool hasKey = table.contains(key);
    if (!hasKey)
    {
      return TResult::Fail(fmt::format("Could not find key '{}'", key));
    }
    bool correctType = table[key].is<T>();
    if (!correctType)
    {
      return TResult::Fail(fmt::format("'{}' is not of the correct type", key));
    }

    T value = table[key].value<T>().value();
    return TResult::Success(value);
  }

  template <typename T, int N>
  static Result<std::array<T, N>, std::string> DeserializeArray(const toml::table& table,
                                                                const std::string& key,
                                                                const T& defaultValue = T())
  {
    using TResult = Result<std::array<T, N>, std::string>;

    bool hasKey = table.contains(key);
    if (!hasKey)
    {
      return TResult::Fail(fmt::format("Could not find key '{}'", key));
    }
    bool correctType = TomlUtils::IsArrayType<T>(table, key);
    if (!correctType)
    {
      return TResult::Fail(fmt::format("'{}' is not of the correct type", key));
    }

    const auto* valueArray = table[key].as_array();
    std::array<T, N> value;
    value.fill(defaultValue);
    int j = 0;
    for (auto i = valueArray->cbegin(); i != valueArray->cend() && j < N; ++i)
    {
      value[j++] = i->template value<T>().value();
    }
    return TResult::Success(value);
  }

  static bool IsInteger(const toml::table& table, const std::string& key)
  {
    return TomlUtils::IsType<int64_t>(table, key);
  }

  static bool IsArrayOfIntegers(const toml::table& table, const std::string& key)
  {
    return TomlUtils::IsArrayType<int64_t>(table, key);
  }

  template <typename T>
  static bool IsType(const toml::table& table, const std::string& key)
  {
    auto node = table.at_path(key);
    return (node && node.is<T>());
  }

  template <typename T>
  static bool IsArrayType(const toml::table& table, const std::string& key)
  {
    auto node = table.at_path(key);
    return (node && node.is_array() && node.is_homogeneous<T>());
  }
};
}
