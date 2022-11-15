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
#ifndef vtk_m_rendering_raytracing_Sampling_h
#define vtk_m_rendering_raytracing_Sampling_h

#include <vtkm/Math.h>
#include <vtkm/VectorAnalysis.h>

namespace vtkm
{
namespace rendering
{
namespace raytracing
{

//
// u0 and u1 are random numbers in [0,1]
//
VTKM_EXEC_CONT
static vtkm::Vec<vtkm::Float32,3> UniformSphereSample(const vtkm::Float32 &u0, const vtkm::Float32 &u1)
{
  vtkm::Float32 z = 1.f - 2.f * u0; // move to [-1,1]
  vtkm::Float32 r = vtkm::Sqrt(vtkm::Max(0.f, 1.f - z*z));
  vtkm::Float32 phi = vtkm::TwoPif() * u1;
  vtkm::Float32 x = r * vtkm::Cos(phi);
  vtkm::Float32 y = r * vtkm::Sin(phi);
  vtkm::Vec<vtkm::Float32,3> res;
  res[0] = x;
  res[1] = y;
  res[2] = z;
  vtkm::Normalize(res);
  return res;
}

//
// Create an orthogal basis (coordinate system) around vector v0
//
VTKM_EXEC_CONT
static void CreateOrthoBasis(const vtkm::Vec<vtkm::Float32,3> &v0,
                             vtkm::Vec<vtkm::Float32,3> &v1,
                             vtkm::Vec<vtkm::Float32,3> &v2)
{
  if(vtkm::Abs(v0[0]) > vtkm::Abs(v0[1]))
  {
    vtkm::Float32 s = 1.f / vtkm::Sqrt(v0[0] * v0[0] + v0[2] * v0[2]);
    v1[0] = -v0[2] * s;
    v1[1] = 0.f;
    v1[2] = v0[0] * s;
  }
  else
  {
    vtkm::Float32 s = 1.f / vtkm::Sqrt(v0[1] * v0[1] + v0[2] * v0[2]);
    v1[0] = 0.f;
    v1[1] = v0[2] * s;
    v1[2] = -v0[1] * s;
  }
  v2 = vtkm::Cross(v0,v1);
}

VTKM_EXEC_CONT
static vtkm::Vec<vtkm::Float32,3> SphericalToDirection(vtkm::Float32 sinTheta,
                                                       vtkm::Float32 cosTheta,
                                                       vtkm::Float32 phi,
                                                       const vtkm::Vec<vtkm::Float32,3> &v0,
                                                       const vtkm::Vec<vtkm::Float32,3> &v1,
                                                       const vtkm::Vec<vtkm::Float32,3> &v2)
{
  vtkm::Vec<vtkm::Float32,3> res;
  res = v0 * sinTheta * vtkm::Cos(phi) + v1 * sinTheta * vtkm::Sin(phi) + cosTheta * v2;
  return res;
}


VTKM_EXEC_CONT
static vtkm::Vec<vtkm::Float32,3> HGPhaseFunction(const vtkm::Vec<vtkm::Float32,3> &wo,
                                                  const vtkm::Float32 g,
                                                  const vtkm::Float32 &u0,
                                                  const vtkm::Float32 &u1,
                                                  vtkm::Float32 &pdf)
{
  // compute sample direction in spherical reference space
  vtkm::Float32 costheta;
  if(vtkm::Abs(g) < 1e-3f)
  {
    costheta = 1.f - 2.f * u0;
  }
  else
  {
    vtkm::Float32 sqr = (1.f - g * g) / (1.f - g + 2.f * g * u0);
    costheta = (1.f + g * g - sqr * sqr) / (2.f * g);
  }

  vtkm::Float32 sintheta = vtkm::Sqrt(vtkm::Max(0.f, 1.f - costheta * costheta));
  vtkm::Float32 phi = 2.f * vtkm::Pif() * u1;
  //
  // Create coordinate system around the incoming direction
  //
  vtkm::Vec<vtkm::Float32,3> v1, v2;
  CreateOrthoBasis(wo, v1, v2);

  vtkm::Vec<vtkm::Float32,3> res = SphericalToDirection(sintheta, costheta, phi, v1, v2, -wo);

  vtkm::Float32 d = 1.f + g * g + 2.f * g * costheta;
  pdf = (1.f - g * g) / (d * vtkm::Sqrt(d) * 4.f * vtkm::Pif());

  return res;
}

}
}
} // namespace vtkm::rendering::raytracing
#endif
