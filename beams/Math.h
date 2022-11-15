#ifndef beams_math_h
#define beams_math_h

#include <vtkm/Bounds.h>
#include <vtkm/Types.h>

namespace beams
{
struct Math
{
public:
  template <typename Precision>
  VTKM_EXEC_CONT static Precision BoundsMagnitude(const vtkm::Bounds& bounds)
  {
    vtkm::Vec<Precision, 3> extent;
    extent[0] = static_cast<Precision>(bounds.X.Length());
    extent[1] = static_cast<Precision>(bounds.Y.Length());
    extent[2] = static_cast<Precision>(bounds.Z.Length());
    return vtkm::Magnitude(extent);
  }
};
} //namespace beams

#endif // beams_math_h