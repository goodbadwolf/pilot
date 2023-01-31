#pragma once

#include <iostream>
#include <string>

namespace pilot
{
enum class ResultTag
{
  Success,
  Failure,
};

template <typename SuccessType, typename FailureType = std::string>
struct Result
{
  using Self = Result<SuccessType, FailureType>;

  ResultTag Tag;
  SuccessType Value;
  FailureType Error;

  Result() = delete;

  bool IsSuccess() const { return this->Tag == ResultTag::Success; }

  bool IsFailure() const { return this->Tag == ResultTag::Failure; }

  explicit operator bool() const { return this->IsSuccess(); }

  static Self Success(const SuccessType& value)
  {
    return Self{ .Tag = ResultTag::Success, .Value = value, .Error = FailureType() };
  }

  static Self Fail(const FailureType& error)
  {
    return Self{ .Tag = ResultTag::Failure, .Value = SuccessType(), .Error = error };
  }
};

using BoolResult = Result<bool, std::string>;
}