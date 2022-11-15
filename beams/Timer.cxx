#include "Timer.h"

#include <map>
#include <vector>

namespace beams
{
Timer::Timer(const std::string& name)
  : Name(name)
{
}

Timer::~Timer() {}

void Timer::Reset()
{
  this->InnerTimer.Reset();
}

void Timer::Start()
{
  this->InnerTimer.Start();
}

void Timer::Stop()
{
  this->InnerTimer.Stop();
}

bool Timer::Started() const
{
  return this->InnerTimer.Started();
}

bool Timer::Stopped() const
{
  return this->InnerTimer.Stopped();
}

bool Timer::Ready() const
{
  return this->InnerTimer.Ready();
}

vtkm::Float64 Timer::GetElapsedTime() const
{
  return this->InnerTimer.GetElapsedTime();
}

} //namespace beams