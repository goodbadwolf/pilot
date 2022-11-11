#pragma once

#include <cstdlib>
#include <iostream>
#include <string>

enum class ResultTag
{
  Valid,
  Error,
};

template <typename ValidResult, typename ErrorType>
struct Result
{
  ResultTag Tag;
  ValidResult Outcome;
  ErrorType Error;

  Result(const ValidResult& result)
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

void Fail(std::string message)
{
  std::cerr << "Failure: " << message << "\n";
  exit(EXIT_FAILURE);
}