#ifndef beams_intersections_h
#define beams_intersections_h

#include <vtkm/Bounds.h>
#include <vtkm/Types.h>

namespace beams
{
namespace Intersections
{
VTKM_EXEC_CONT bool SegmentAABB(const vtkm::Vec3f_32& start,
                                const vtkm::Vec3f_32& end,
                                vtkm::Bounds aabb,
                                vtkm::Float32& tMin,
                                vtkm::Float32& tMax,
                                vtkm::Float32 tEps = 1e-4f);

template <typename Precision>
VTKM_EXEC_CONT bool ApproxEquals(Precision x, Precision y, Precision eps = 1e-5f)
{
  return vtkm::Abs(x - y) <= eps;
}
}
} // namespace beams::Intersections

#endif // beams_intersections_h