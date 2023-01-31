#ifndef beams_result_h
#define beams_result_h

#include <string>

namespace beams
{
#define CHECK_RESULT_BEAMS(statement, errPrefix)                           \
  {                                                                        \
    auto result = (statement);                                             \
    if (!result.Success)                                                   \
    {                                                                      \
      return Result::Failed(fmt::format("{}: {}", errPrefix, result.Err)); \
    }                                                                      \
  }

struct Result
{
  bool Success = false;
  std::string Err = "Unknown";

  static inline Result Failed(const std::string& err)
  {
    return Result{ .Success = false, .Err = err };
  }

  static inline Result Succeeded() { return Result{ .Success = true }; }
};

} // namespace beams

#endif // beams_result_h
