#pragma once

#include "Result.h"

#include <vtkm/cont/DataSet.h>
#include <vtkm/filter/FieldSelection.h>

using FilterResult = Result<vtkm::cont::DataSet, std::string>;

FilterResult FilterFields(vtkm::cont::DataSet& dataSet, vtkm::filter::FieldSelection fieldSelection)
{
  vtkm::cont::DataSet filteredDataSet;

  // Copy over coordinate systems from original
  for (const auto& coordSystem : dataSet.GetCoordinateSystems())
  {
    filteredDataSet.AddCoordinateSystem(coordSystem);
  }

  // Copy over cellset from original
  filteredDataSet.SetCellSet(dataSet.GetCellSet());

  // Copy over selected fields
  for (auto i = 0; i < dataSet.GetNumberOfFields(); ++i)
  {
    const auto& field = dataSet.GetField(i);
    if (fieldSelection.HasField(field))
    {
      filteredDataSet.AddField(field);
    }
  }

  return FilterResult(filteredDataSet);
}