#include "Transform.h"
#include "Chunk.h"

#include <pilot/Debug.h>

#include <vtkm/filter/field_transform/PointTransform.h>

namespace pilot
{
namespace apps
{
namespace convert
{
TransformResult TransformDataSet(const vtkm::cont::DataSet& dataSet,
                                 const vtkm::Vec3f_32& translation,
                                 vtkm::filter::FieldSelection fields)
{
  const std::string PREFIX = std::string("*** ") + PILOT_CURRENT_FUNC_NAME + std::string(" ***");
  std::cerr << PREFIX << "\n";
  std::cerr << "Before transformation\n";
  std::cerr << "Bounds: " << dataSet.GetCoordinateSystem().GetBounds() << "\n";
  // dataSet.GetCoordinateSystem().PrintSummary(std::cerr);

  vtkm::filter::field_transform::PointTransform translateFilter;
  translateFilter.SetTranslation(translation);
  auto transformedDataSet = translateFilter.Execute(dataSet);
  transformedDataSet = FilterFields(transformedDataSet, fields).Value;
  std::cerr << "After translation\n";
  // transformedDataSet.GetCoordinateSystem().PrintSummary(std::cerr);
  std::cerr << "Bounds: " << transformedDataSet.GetCoordinateSystem().GetBounds() << "\n";
  transformedDataSet.PrintSummary(std::cerr);

  vtkm::filter::field_transform::PointTransform scaleFilter;
  vtkm::Vec3f_32 scale{ 0.00401569332f, 0.00311915159f, 0.00401569332f };
  scaleFilter.SetScale(scale);
  transformedDataSet = scaleFilter.Execute(transformedDataSet);
  transformedDataSet = FilterFields(transformedDataSet, fields).Value;
  std::cerr << "After scaling\n";
  // transformedDataSet.GetCoordinateSystem().PrintSummary(std::cerr);
  std::cerr << "Bounds: " << transformedDataSet.GetCoordinateSystem().GetBounds() << "\n";

  // std::cerr << "After transformation\n";
  // transformedDataSet.GetCoordinateSystem().PrintSummary(std::cerr);
  return TransformResult::Success(transformedDataSet);
}
}
}
}