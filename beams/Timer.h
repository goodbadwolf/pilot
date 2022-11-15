#ifndef beams_timer_h
#define beams_timer_h

#include <vtkm/cont/Timer.h>

#include <string>

namespace beams
{
struct Timer
{
public:
  VTKM_CONT Timer(const std::string& name);

  VTKM_CONT ~Timer();

  VTKM_CONT void Reset();

  VTKM_CONT void Start();

  VTKM_CONT void Stop();

  VTKM_CONT bool Started() const;

  VTKM_CONT bool Stopped() const;

  VTKM_CONT bool Ready() const;

  VTKM_CONT
  vtkm::Float64 GetElapsedTime() const;

  std::string Name;

private:
  vtkm::cont::Timer InnerTimer;
}; // struct Timer
} // namespace beams

#endif // beams_timer_h
