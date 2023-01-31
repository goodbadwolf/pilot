#include <pilot/Logger.h>
#include <pilot/system/SystemUtils.h>

#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/RuntimeDeviceInformation.h>

#include <cstdlib>
#include <iostream>

namespace pilot
{
namespace system
{
void Die(const std::string& message)
{
  std::cerr << "Failure: " << message << "\n";
  exit(EXIT_FAILURE);
}

void PrintVtkmDeviceInfo()
{
#ifdef VTKM_ENABLE_OPENMP
  auto& runtimeConfig = vtkm::cont::RuntimeDeviceInformation{}.GetRuntimeConfiguration(
    vtkm::cont::DeviceAdapterTagOpenMP());
  vtkm::Id maxThreads = 0;
  vtkm::Id numThreads = 0;
  runtimeConfig.GetMaxThreads(maxThreads);
  runtimeConfig.GetThreads(numThreads);
  LOG::Info("VTKm Device: OpenMP");
  LOG::Info("MaxThreads: {}", maxThreads);
  LOG::Info("NumThreads: {}", numThreads);
#endif
}
}
}