#ifndef beams_datasetfactory_h
#define beams_datasetfactory_h

#include "Config.h"
#include "Result.h"

#include <vtkm/cont/DataSet.h>

namespace beams
{

class DataSetFactory
{
public:
  static beams::Result CreateDataSetFromPreset(const beams::Preset& preset,
                                                 vtkm::cont::DataSet& dataSet,
                                                 std::string& fieldName);
};

} // namespace beams
#endif // beams_datasetfactory_h