#ifndef beams_mapper_volume_shadow_h
#define beams_mapper_volume_shadow_h

#include "../Profiler.h"
#include "BoundsMap.h"
#include "Light.h"

#include <vtkm/rendering/Mapper.h>

#include <memory>

namespace beams
{
namespace rendering
{
class MapperLightedVolume : public vtkm::rendering::Mapper
{
public:
  MapperLightedVolume();

  ~MapperLightedVolume();

  void SetCanvas(vtkm::rendering::Canvas* canvas) override;
  virtual vtkm::rendering::Canvas* GetCanvas() const override;

  void SetBoundsMap(beams::rendering::BoundsMap* boundsMap);

  VTKM_CONT
  void SetShadowMapSize(vtkm::Id3 size);

  void SetUseShadowMap(bool useShadowMap);

  virtual void RenderCells(const vtkm::cont::UnknownCellSet& cellset,
                           const vtkm::cont::CoordinateSystem& coords,
                           const vtkm::cont::Field& scalarField,
                           const vtkm::cont::ColorTable&, //colorTable
                           const vtkm::rendering::Camera& camera,
                           const vtkm::Range& scalarRange) override;

  vtkm::rendering::Mapper* NewCopy() const override;
  void SetSampleDistance(const vtkm::Float32 distance);
  void SetCompositeBackground(const bool compositeBackground);

  VTKM_CONT
  void AddLight(std::shared_ptr<Light> light);

  VTKM_CONT
  void ClearLights();

  VTKM_CONT
  void SetDensityScale(vtkm::Float32 densityScale);

  VTKM_CONT
  void GlobalComposite(const vtkm::rendering::Camera& camera);

  VTKM_CONT
  void SetProfiler(std::shared_ptr<beams::Profiler> profiler);

private:
  struct InternalsType;
  std::shared_ptr<InternalsType> Internals;
};
} // namespace rendering
} //namespace beams

#endif // beams_MapperVolume_h
