//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_rendering_raytracing_Random_h
#define vtk_m_rendering_raytracing_Random_h

#include <vtkm/cont/ArrayHandle.h>

#include <stdlib.h>
#include <time.h>

namespace vtkm
{
namespace rendering
{
namespace raytracing
{
namespace detail
{
union UIntFloat
{
  vtkm::UInt32 U;
  vtkm::Float32 F;
};
} //  namespace detail
// multiple with cary random numbers
VTKM_EXEC_CONT
static vtkm::UInt32 random(vtkm::Vec<vtkm::UInt32, 2>& rngState)
{
  rngState[0] = 36969 * (rngState[0] & 65535) + (rngState[0] >> 16);
  rngState[1] = 18000 * (rngState[1] & 65535) + (rngState[1] >> 16);

  vtkm::UInt32 res = (rngState[0] << 16) + rngState[1];
  return res;
}

VTKM_EXEC_CONT
static vtkm::Float32 randomf(vtkm::Vec<vtkm::UInt32, 2>& rngState)
{
  detail::UIntFloat r;
  vtkm::UInt32 res = random(rngState);
  r.U = (res & 0x007fffff) | 0x40000000;
  return (r.F - 2.f) / 2.f;
}

VTKM_CONT
static void seedRng(vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::UInt32, 2>>& rngStates)
{
  const vtkm::Id size = rngStates.GetNumberOfValues();
  auto portal = rngStates.WritePortal();
  //srand ((vtkm::UInt32(time(NULL))));
  srand(0);

  for (vtkm::Id i = 0; i < size; ++i)
  {
    vtkm::Vec<vtkm::UInt32, 2> state;
    state[0] = static_cast<vtkm::UInt32>(rand());
    state[1] = static_cast<vtkm::UInt32>(rand());
    portal.Set(i, state);
  }
}

}
}
} // namespace vtkm::rendering::raytracing
#endif //vtk_m_rendering_raytracing_Random_h
