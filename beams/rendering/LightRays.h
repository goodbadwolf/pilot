#ifndef beams_rendering_lightrays_h
#define beams_rendering_lightrays_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCompositeVector.h>

namespace beams
{
namespace rendering
{
template <typename Precision, typename Device>
struct LightRays
{
public:
  using PrecisionHandle = vtkm::cont::ArrayHandle<Precision>;
  using CompositeHandle =
    vtkm::cont::ArrayHandleCompositeVector<PrecisionHandle, PrecisionHandle, PrecisionHandle>;

  VTKM_CONT
  LightRays()
    : LightRays(0)
  {
  }

  VTKM_CONT
  LightRays(vtkm::Id count) { this->Resize(count); }

  VTKM_CONT void Resize(vtkm::Id count)
  {
    this->Count = count;
    this->Origins = vtkm::cont::make_ArrayHandleCompositeVector(OriginXs, OriginYs, OriginZs);
    this->Dirs = vtkm::cont::make_ArrayHandleCompositeVector(DirXs, DirYs, DirZs);
    this->Dests = vtkm::cont::make_ArrayHandleCompositeVector(DestXs, DestYs, DestZs);
  }

  vtkm::Id Count;

  vtkm::cont::ArrayHandle<vtkm::Id> Ids;

  PrecisionHandle OriginXs;
  PrecisionHandle OriginYs;
  PrecisionHandle OriginZs;
  CompositeHandle Origins;

  PrecisionHandle DirXs;
  PrecisionHandle DirYs;
  PrecisionHandle DirZs;
  CompositeHandle Dirs;

  PrecisionHandle DestXs;
  PrecisionHandle DestYs;
  PrecisionHandle DestZs;
  CompositeHandle Dests;
}; // struct LightRays
} // namespace rendering
} // namespace beams

#endif // beams_rendering_lightrays_h