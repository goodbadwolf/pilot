#include "Spheres.h"
#include "../utils/Fmt.h"

#include <vtkm/VectorAnalysis.h>
#include <vtkm/filter/FilterField.h>
#include <vtkm/worklet/WorkletMapField.h>

using CartesianCoordinateHandle = vtkm::cont::ArrayHandle<vtkm::Float32>;
using CartesianProductHandle = vtkm::cont::ArrayHandleCartesianProduct<CartesianCoordinateHandle,
                                                                       CartesianCoordinateHandle,
                                                                       CartesianCoordinateHandle>;

namespace beams
{
namespace sources
{
namespace
{
struct SphereFieldGenerator : public vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn points,
                                WholeArrayIn centers,
                                WholeArrayIn radii,
                                WholeArrayIn fieldValues,
                                FieldOut sphereData);
  using ExecutionSignature = void(_1 points, _2 centers, _3 radii, _4 fieldValues, _5 sphereData);

  VTKM_CONT SphereFieldGenerator(const vtkm::Float32& defaultValue, bool hasFloor)
    : DefaultValue(defaultValue)
    , HasFloor(hasFloor)
  {
  }

  template <typename PointVecType,
            typename CenterPortal,
            typename RadiiPortal,
            typename FieldValuesPortal>
  VTKM_EXEC void operator()(const PointVecType& point,
                            const CenterPortal& centers,
                            const RadiiPortal& radii,
                            const FieldValuesPortal& fieldValues,
                            vtkm::Float32& sphereData) const
  {
    sphereData = 0.0f;
    vtkm::Float32 numIntersectingEllipsoids = 0.0f;

    auto numEllipsoids = centers.GetNumberOfValues() - 1;
    if (this->HasFloor)
      numEllipsoids -= 1;

    for (vtkm::Id i = 0; i <= numEllipsoids; ++i)
    {
      auto center = centers.Get(i);
      auto radius = radii.Get(i);
      auto fieldValueRange = fieldValues.Get(i);
      auto minFieldValue = fieldValueRange[0];
      auto maxFieldValue = fieldValueRange[1];

      auto pointToCenter = center - point;
      auto affineDist = ((pointToCenter[0] * pointToCenter[0]) / (radius[0] * radius[0])) +
        ((pointToCenter[1] * pointToCenter[1]) / (radius[1] * radius[1])) +
        ((pointToCenter[2] * pointToCenter[2]) / (radius[2] * radius[2]));
      affineDist = vtkm::Sqrt(affineDist);

      bool isInsideEllipsoid = affineDist <= 1.0f;
      if (isInsideEllipsoid)
      {
        vtkm::Float32 density = vtkm::Lerp(minFieldValue, maxFieldValue, affineDist);
        sphereData += density;
        numIntersectingEllipsoids += 1.0f;
      }
    }

    if (numIntersectingEllipsoids > 0.0f)
    {
      sphereData = sphereData / numIntersectingEllipsoids;
    }
    else
    {
      sphereData = this->DefaultValue;
    }

    if (this->HasFloor)
    {
      auto floorY = centers.Get(centers.GetNumberOfValues() - 1)[1];
      auto floorValue = fieldValues.Get(centers.GetNumberOfValues() - 1)[0];
      if (point[1] < floorY)
      {
        sphereData = floorValue;
      }
    }
  }

  vtkm::Float32 DefaultValue;
  bool HasFloor;
};

class SphereField : public vtkm::filter::FilterField<SphereField>
{
public:
  VTKM_CONT SphereField(const vtkm::Float32& defaultFieldValue,
                        const vtkm::cont::ArrayHandle<vtkm::Vec3f>& centers,
                        const vtkm::cont::ArrayHandle<vtkm::Vec3f>& radii,
                        const vtkm::cont::ArrayHandle<vtkm::Vec2f>& fieldValues,
                        bool hasFloor)
    : DefaultFieldValue(defaultFieldValue)
    , Centers(centers)
    , Radii(radii)
    , FieldValues(fieldValues)
    , HasFloor(hasFloor)
  {
    this->SetUseCoordinateSystemAsField(true);
  }

  template <typename FieldType, typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet DoExecute(
    const vtkm::cont::DataSet& input,
    const FieldType& vtkmNotUsed(field),
    const vtkm ::filter::FieldMetadata& fieldMetadata,
    vtkm::filter::PolicyBase<DerivedPolicy> vtkmNotUsed(policy))
  {
    vtkm::cont::ArrayHandle<vtkm::Float32> sphereFieldData;
    SphereFieldGenerator generator{ this->DefaultFieldValue, this->HasFloor };
    this->Invoke(generator,
                 input.GetCoordinateSystem(),
                 this->Centers,
                 this->Radii,
                 this->FieldValues,
                 sphereFieldData);

    return vtkm::filter::CreateResult(
      input, sphereFieldData, this->GetOutputFieldName(), fieldMetadata);
  }

private:
  vtkm::Float32 DefaultFieldValue;
  vtkm::cont::ArrayHandle<vtkm::Vec3f> Centers;
  vtkm::cont::ArrayHandle<vtkm::Vec3f> Radii;
  vtkm::cont::ArrayHandle<vtkm::Vec2f> FieldValues;
  bool HasFloor;
};

vtkm::cont::ArrayHandleUniformPointCoordinates BuildUniformCoordinates(vtkm::Id3 dims,
                                                                       vtkm::Vec3f_32 origin,
                                                                       vtkm::Vec3f_32 spacing)
{
  return vtkm::cont::ArrayHandleUniformPointCoordinates(dims, origin, spacing);
}

CartesianProductHandle BuildRectilinearCoordiates(vtkm::Id3 dims,
                                                  vtkm::Vec3f_32 origin,
                                                  vtkm::Vec3f_32 spacing)
{
  CartesianCoordinateHandle x, y, z;
  vtkm::cont::Algorithm::Copy(
    vtkm::cont::ArrayHandleCounting<vtkm::Float32>(origin[0], spacing[0], dims[0]), x);
  vtkm::cont::Algorithm::Copy(
    vtkm::cont::ArrayHandleCounting<vtkm::Float32>(origin[1], spacing[1], dims[1]), y);
  vtkm::cont::Algorithm::Copy(
    vtkm::cont::ArrayHandleCounting<vtkm::Float32>(origin[2], spacing[2], dims[2]), z);
  return vtkm::cont::make_ArrayHandleCartesianProduct(x, y, z);
}
} // namespace

Spheres::Spheres(vtkm::Vec3f_32 origin,
                 vtkm::Id3 dims,
                 vtkm::Vec3f_32 spacing,
                 vtkm::Float32 defaultFieldValue,
                 CoordinatesType coordsType)
  : Origin(origin)
  , Dims(dims)
  , Spacing(spacing)
  , DefaultFieldValue(defaultFieldValue)
  , CoordsType(coordsType)
{
}

void Spheres::AddSphere(vtkm::Vec3f_32 center,
                        vtkm::Float32 radius,
                        vtkm::Float32 minFieldValue,
                        vtkm::Float32 maxFieldValue)
{
  this->AddEllipsoid(center, radius, radius, radius, minFieldValue, maxFieldValue);
}

void Spheres::AddEllipsoid(vtkm::Vec3f_32 center,
                           vtkm::Float32 radiusA,
                           vtkm::Float32 radiusB,
                           vtkm::Float32 radiusC,
                           vtkm::Float32 minFieldValue,
                           vtkm::Float32 maxFieldValue)
{
  this->Centers.push_back(center);
  this->Radii.push_back({ radiusA, radiusB, radiusC });
  this->FieldValues.push_back({ minFieldValue, maxFieldValue });
}

void Spheres::SetFloor(vtkm::Float32 height, vtkm::Float32 value)
{
  vtkm::Vec3f_32 center{
    this->Origin[0],
    this->Origin[1] + height,
    this->Origin[2],
  };
  this->HasFloor = true;
  this->AddSphere(center, 0.0f, value, value);
}

vtkm::cont::DataSet Spheres::DoExecute() const
{
  const vtkm::Id3 pdims{ this->Dims + vtkm::Id3{ 1, 1, 1 } };
  vtkm::cont::CellSetStructured<3> cellSet;
  cellSet.SetPointDimensions(pdims);

  vtkm::cont::DataSet dataSet;
  dataSet.SetCellSet(cellSet);
  switch (this->CoordsType)
  {
    case CoordinatesType::Uniform:
      dataSet.AddCoordinateSystem(vtkm::cont::CoordinateSystem(
        "coordinates", BuildUniformCoordinates(pdims, this->Origin, this->Spacing)));
      break;
    case CoordinatesType::Rectilinear:
      dataSet.AddCoordinateSystem(vtkm::cont::CoordinateSystem(
        "coordinates", BuildRectilinearCoordiates(pdims, this->Origin, this->Spacing)));
      break;
  }
  vtkm::cont::ArrayHandle<vtkm::Vec3f> centers =
    vtkm::cont::make_ArrayHandle(this->Centers, vtkm::CopyFlag::On);
  vtkm::cont::ArrayHandle<vtkm::Vec3f> radii =
    vtkm::cont::make_ArrayHandle(this->Radii, vtkm::CopyFlag::On);
  vtkm::cont::ArrayHandle<vtkm::Vec2f> fieldValues =
    vtkm::cont::make_ArrayHandle(this->FieldValues, vtkm::CopyFlag::On);

  SphereField field{ this->DefaultFieldValue, centers, radii, fieldValues, this->HasFloor };
  field.SetOutputFieldName("sphere");
  dataSet = field.Execute(dataSet);

  return dataSet;
}

} // namespace sources
} // namespace beams