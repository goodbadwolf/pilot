#ifndef beams_logger_h
#define beams_logger_h

#include "Timer.h"

#include <map>
#include <stack>
#include <string>
#include <vector>

namespace beams
{
struct ProfilerFrame
{
  ProfilerFrame(const std::string& name);

  void Collect();

  void PrintSummary(std::ostream& stream, int level) const;

  std::string Name;
  beams::Timer Timer;
  bool Distributed;
  std::vector<vtkm::Float64> ElapsedTimes;
  vtkm::Float64 MinTime;
  vtkm::Float64 MaxTime;
  vtkm::Float64 AvgTime;
  std::vector<std::shared_ptr<ProfilerFrame>> SubFrames;
}; // struct ProfilerFrame

class Profiler
{
public:
  Profiler(const std::string& name);

  void StartFrame(const std::string& name);

  void EndFrame();

  void Collect();

  void PrintSummary(std::ostream& stream);

protected:
  ProfilerFrame CreateNewFrame(const std::string& name);

  void Push(std::shared_ptr<ProfilerFrame> frame);

  std::shared_ptr<ProfilerFrame> Pop();

  std::shared_ptr<ProfilerFrame> Peek();

  bool Empty() const;

  void PrintSummaryInternal(std::shared_ptr<ProfilerFrame> frame, std::ostream& stream, int level);

  void CollectInternal(std::shared_ptr<ProfilerFrame> frame);

  std::shared_ptr<ProfilerFrame> RootFrame;
  std::stack<std::shared_ptr<ProfilerFrame>> FrameStack;
}; // class Profiler
} // namespace beams

#endif // beams_logger_h