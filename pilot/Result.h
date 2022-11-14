#pragma once

#include <cstdlib>
#include <iostream>
#include <string>

namespace pilot
{
enum class ResultTag
{
  Valid,
  Error,
};

template <typename ResultType, typename ErrorType>
struct Result
{
  ResultTag Tag;
  ResultType Outcome;
  ErrorType Error;

  Result(const ResultType& result)
    : Tag(ResultTag::Valid)
    , Outcome(result)
  {
  }

  Result(const ErrorType& error)
    : Tag(ResultTag::Error)
    , Error(error)
  {
  }

  bool IsValid() const { return this->Tag == ResultTag::Valid; }

  bool IsError() const { return this->Tag == ResultTag::Error; }
};
}