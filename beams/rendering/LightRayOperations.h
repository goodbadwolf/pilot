#ifndef beams_rendering_lightrayoperations_h
#define beams_rendering_lightrayoperations_h

#include "LightRays.h"

#include <vtkm/VectorAnalysis.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/Invoker.h>
#include <vtkm/worklet/WorkletMapField.h>

namespace beams
{
namespace rendering
{
namespace detail
{
template <typename Precision>
struct LightRaysGenerator : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature =
    void(FieldIn points, FieldOut rayIds, FieldOut origins, FieldOut dirs, FieldOut dests);
  using ExecutionSignature = void(_1, InputIndex, _2, _3, _4, _5);
  using Vec3 = vtkm::Vec<Precision, 3>;

  VTKM_CONT
  LightRaysGenerator(const Vec3& lightPosition)
    : LightPosition(lightPosition)
  {
  }

  VTKM_EXEC void operator()(const Vec3& point,
                            const vtkm::Id inputIndex,
                            vtkm::Id& rayId,
                            Vec3& origin,
                            Vec3& dir,
                            Vec3& dest) const
  {
    rayId = inputIndex;
    origin = this->LightPosition;
    dest = point;
    dir = vtkm::Normal(dest - origin);
  }

  Vec3 LightPosition;
}; // struct LightRaysGenerator
} // namespace detail

struct LightRayOperations
{
  template <typename Precision, typename PointsArrayHandle, typename Device>
  static LightRays<Precision, Device> CreateRays(const PointsArrayHandle& points,
                                                 const vtkm::Vec<Precision, 3>& lightPosition)
  {
    vtkm::Id count = points.GetNumberOfValues();
    vtkm::cont::Invoker invoker;
    LightRays<Precision, Device> rays(count);
    invoker(detail::LightRaysGenerator<Precision>{ lightPosition },
            points,
            rays.Ids,
            rays.Origins,
            rays.Dirs,
            rays.Dests);
    return rays;
  }

}; // struct LightRayOperations
} // namespace rendering
} // namespace beams

#endif // beams_rendering_lightrayoperations_h