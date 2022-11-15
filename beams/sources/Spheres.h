#ifndef beams_sources_spheres_h
#define beams_sources_spheres_h

#include "CoordinatesType.h"

#include <vector>
#include <vtkm/Bounds.h>
#include <vtkm/source/Source.h>

namespace beams
{
namespace sources
{
class Spheres : public vtkm::source::Source
{
public:
  VTKM_CONT
  Spheres(vtkm::Vec3f_32 origin,
          vtkm::Id3 dims,
          vtkm::Vec3f_32 spacing,
          vtkm::Float32 defaultFieldValue = 0.1f,
          CoordinatesType coordsType = CoordinatesType::Uniform);

  VTKM_CONT
  void AddSphere(vtkm::Vec3f_32 center,
                 vtkm::Float32 radius,
                 vtkm::Float32 minFieldValue,
                 vtkm::Float32 maxFieldValue);

  VTKM_CONT
  void AddEllipsoid(vtkm::Vec3f center,
                    vtkm::Float32 radiusA,
                    vtkm::Float32 radiusB,
                    vtkm::Float32 radiusC,
                    vtkm::Float32 minFieldValue,
                    vtkm::Float32 maxFieldValue);

  VTKM_CONT
  void SetFloor(vtkm::Float32 height, vtkm::Float32 floorValue);

protected:
  vtkm::cont::DataSet DoExecute() const override;

private:
  vtkm::Vec3f_32 Origin;
  vtkm::Id3 Dims;
  vtkm::Vec3f_32 Spacing;
  vtkm::Float32 DefaultFieldValue;
  std::vector<vtkm::Vec3f_32> Centers;
  std::vector<vtkm::Vec3f_32> Radii;
  std::vector<vtkm::Vec2f_32> FieldValues;
  bool HasFloor = false;
  beams::sources::CoordinatesType CoordsType;
}; // class Spheres
}
} // namespace beams::sources

#endif // beams_sources_spheres_h
