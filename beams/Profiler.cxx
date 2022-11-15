#include "Profiler.h"
#include "mpi/MpiEnv.h"
#include "utils/Fmt.h"

#include <vtkm/thirdparty/diy/diy.h>
#include <vtkm/thirdparty/diy/mpi-cast.h>

namespace
{
int ToMs(vtkm::Float64 t)
{
  return static_cast<int>(t * 1000);
}
} // namespace

namespace beams
{
ProfilerFrame::ProfilerFrame(const std::string& name)
  : Name(name)
  , Timer(name)
{
}

void ProfilerFrame::Collect()
{
  auto mpi = beams::mpi::MpiEnv::Get();
  auto comm = mpi->Comm;
  MPI_Comm mpiComm = vtkmdiy::mpi::mpi_cast(comm->handle());
  this->ElapsedTimes.resize(mpi->Size);
  vtkm::Float64 rankTime = this->Timer.GetElapsedTime();
  MPI_Gather(&rankTime, 1, MPI_DOUBLE, this->ElapsedTimes.data(), 1, MPI_DOUBLE, 0, mpiComm);
  if (mpi->Rank == 0)
  {
    auto minMaxTime = std::minmax_element(this->ElapsedTimes.begin(), this->ElapsedTimes.end());
    this->MinTime = *(minMaxTime.first);
    this->MaxTime = *(minMaxTime.second);
    this->AvgTime = std::accumulate(this->ElapsedTimes.begin(), this->ElapsedTimes.end(), 0.0) /
      static_cast<vtkm::Float64>(this->ElapsedTimes.size());
  }
}

void ProfilerFrame::PrintSummary(std::ostream& stream, int level) const
{
  std::string spaces(level * 2, ' ');
  std::string summary = fmt::format("{}{}: Min = {} ms, Max = {} ms, Avg = {} ms\n",
                                    spaces,
                                    this->Name,
                                    ToMs(this->MinTime),
                                    ToMs(this->MaxTime),
                                    ToMs(this->AvgTime));
  stream << summary;
}

Profiler::Profiler(const std::string& name)
{
  this->StartFrame(name);
}

void Profiler::StartFrame(const std::string& name)
{
  std::shared_ptr<ProfilerFrame> frame = std::make_shared<ProfilerFrame>(name);
  if (!this->Empty())
  {
    auto topFrame = this->Peek();
    topFrame->SubFrames.push_back(frame);
    this->Push(frame);
  }
  else
  {
    this->RootFrame = frame;
    this->Push(frame);
  }
  frame->Timer.Start();
}

void Profiler::EndFrame()
{
  auto topFrame = this->Pop();
  topFrame->Timer.Stop();
}

void Profiler::Collect()
{
  this->CollectInternal(this->RootFrame);
}

void Profiler::CollectInternal(std::shared_ptr<ProfilerFrame> frame)
{
  frame->Collect();
  for (auto subFrame : frame->SubFrames)
  {
    this->CollectInternal(subFrame);
  }
}

void Profiler::Push(std::shared_ptr<ProfilerFrame> frame)
{
  this->FrameStack.push(frame);
}

std::shared_ptr<ProfilerFrame> Profiler::Peek()
{
  return this->FrameStack.top();
}

std::shared_ptr<ProfilerFrame> Profiler::Pop()
{
  auto topFrame = this->Peek();
  this->FrameStack.pop();
  return topFrame;
}

bool Profiler::Empty() const
{
  return this->FrameStack.empty();
}

void Profiler::PrintSummary(std::ostream& stream)
{
  int level = 0;
  this->PrintSummaryInternal(this->RootFrame, stream, level);
}

void Profiler::PrintSummaryInternal(std::shared_ptr<ProfilerFrame> frame,
                                    std::ostream& stream,
                                    int level)
{
  frame->PrintSummary(stream, level);
  for (auto& subFrame : frame->SubFrames)
  {
    this->PrintSummaryInternal(subFrame, stream, level + 1);
  }
}
} // namespace beams