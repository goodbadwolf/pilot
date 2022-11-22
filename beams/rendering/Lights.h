//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//
//=============================================================================
#ifndef vtk_m_rendering_raytracing_Lights_h
#define vtk_m_rendering_raytracing_Lights_h

#include "Random.h"
#include "Sampling.h"
#include <vtkm/Math.h>
#include <vtkm/Matrix.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/rendering/raytracing/Ray.h>
#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace rendering
{
namespace raytracing
{

template <typename Device>
class LightsExec
{
public:
  using FloatHandle = vtkm::cont::ArrayHandle<vtkm::Float32>;
  using Float4Handle = vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 3>>;
  using FloatPortal = typename FloatHandle::ReadPortalType;
  using Float4Portal = typename Float4Handle::ReadPortalType;
  FloatPortal Radii;
  Float4Portal Locations;
  Float4Portal Colors;
  vtkm::Id NumLights;

public:
  LightsExec() {}
  LightsExec(FloatHandle radii,
             Float4Handle locations,
             Float4Handle colors,
             Device,
             vtkm::cont::Token& token)
  {
    Radii = radii.PrepareForInput(Device(), token);
    Locations = locations.PrepareForInput(Device(), token);
    Colors = colors.PrepareForInput(Device(), token);
    NumLights = Radii.GetNumberOfValues();
  }

  VTKM_EXEC
  vtkm::Float32 IntersectLights(const vtkm::Vec<vtkm::Float32, 3>& origin,
                                const vtkm::Vec<vtkm::Float32, 3>& direction) const
  {
    vtkm::Float32 distance = vtkm::Infinity32();

    for (int i = 0; i < NumLights; ++i)
    {
      vtkm::Vec<vtkm::Float32, 3> lpos = Locations.Get(i);
      vtkm::Float32 lradius = Radii.Get(i);

      // l is the vector from ray origin (camera loc) to light position
      vtkm::Vec<vtkm::Float32, 3> l = lpos - origin;

      vtkm::Float32 dot1 = vtkm::dot(l, direction);
      if (dot1 >= 0)
      {
        // Angle between l and direction in in range [0, 90]
        vtkm::Float32 d = vtkm::dot(l, l) - dot1 * dot1;
        vtkm::Float32 r2 = lradius * lradius;
        if (d <= r2)
        {
          vtkm::Float32 tch = vtkm::Sqrt(r2 - d);
          vtkm::Float32 tmp = dot1 - tch;
          distance = vtkm::Min(tmp, distance);
        }
      }
    }
    return distance;
  }

  VTKM_EXEC
  bool IntersectLights(const vtkm::Vec<vtkm::Float32, 3>& origin,
                       const vtkm::Vec<vtkm::Float32, 3>& direction,
                       vtkm::Vec<vtkm::Float32, 3>& color) const
  {
    vtkm::Float32 distance = vtkm::Infinity32();

    for (int i = 0; i < NumLights; ++i)
    {
      vtkm::Vec<vtkm::Float32, 3> lpos = Locations.Get(i);
      vtkm::Float32 lradius = Radii.Get(i);

      vtkm::Vec<vtkm::Float32, 3> l = lpos - origin;

      vtkm::Float32 dot1 = vtkm::dot(l, direction);
      if (dot1 >= 0)
      {
        vtkm::Float32 d = vtkm::dot(l, l) - dot1 * dot1;
        vtkm::Float32 r2 = lradius * lradius;
        if (d <= r2)
        {
          vtkm::Float32 tch = vtkm::Sqrt(r2 - d);
          vtkm::Float32 tmp = dot1 - tch;
          if (tmp < distance)
          {
            color = Colors.Get(i);
            distance = tmp;
          }
        }
      }
    }

    return distance != vtkm::Infinity32();
  }

  VTKM_EXEC
  void SampleLight(const vtkm::Vec<vtkm::Float32, 3>& p,
                   vtkm::Vec<vtkm::UInt32, 2>& rng,
                   vtkm::Vec<vtkm::Float32, 3>& direction,
                   vtkm::Float32& distance) const
  {
    const vtkm::Id lightId = random(rng) % NumLights;
    vtkm::Vec<vtkm::Float32, 3> lpos = Locations.Get(lightId);
    vtkm::Float32 lradius = Radii.Get(lightId);
    // https://schuttejoe.github.io/post/arealightsampling/

    vtkm::Vec<vtkm::Float32, 3> ldir = lpos - p;
    vtkm::Float32 lmag = vtkm::Magnitude(ldir);
    /*
    if (lmag != lmag)
      std::cout << "bb";
    */
    ldir = ldir * (1.f / lmag);
    vtkm::Float32 r_lmag = lradius / lmag;
    vtkm::Float32 q = vtkm::Sqrt(1.f - r_lmag * r_lmag);

    vtkm::Vec<vtkm::Float32, 3> u, v;
    CreateOrthoBasis(ldir, v, u);

    vtkm::Matrix<vtkm::Float32, 4, 4> toWorld;
    vtkm::MatrixIdentity(toWorld);

    toWorld(0, 0) = u[0];
    toWorld(0, 1) = u[1];
    toWorld(0, 2) = u[2];
    toWorld(1, 0) = ldir[0];
    toWorld(1, 1) = ldir[1];
    toWorld(1, 2) = ldir[2];
    toWorld(2, 0) = v[0];
    toWorld(2, 1) = v[1];
    toWorld(2, 2) = v[2];

    vtkm::Float32 r0 = randomf(rng);
    vtkm::Float32 r1 = randomf(rng);
    vtkm::Float32 theta = vtkm::ACos(1.f - r0 + r0 * q);
    vtkm::Float32 phi = vtkm::TwoPif() * r1;
    // convert to cartesian
    vtkm::Float32 sinTheta = vtkm::Sin(theta);
    vtkm::Float32 cosTheta = vtkm::Cos(theta);
    vtkm::Vec<vtkm::Float32, 4> local;
    local[0] = vtkm::Cos(phi) * sinTheta;
    local[1] = cosTheta;
    local[2] = vtkm::Sin(phi) * cosTheta;
    local[3] = 0;

    vtkm::Vec<vtkm::Float32, 4> sampleDir;
    sampleDir = vtkm::MatrixMultiply(local, toWorld);
    direction[0] = sampleDir[0];
    direction[1] = sampleDir[1];
    direction[2] = sampleDir[2];

    vtkm::Normalize(direction);

    vtkm::Vec<vtkm::Float32, 3> l = lpos - p;

    vtkm::Float32 dot1 = vtkm::dot(l, direction);
    if (dot1 >= 0)
    {
      vtkm::Float32 d = vtkm::dot(l, l) - dot1 * dot1;
      vtkm::Float32 r2 = lradius * lradius;
      if (d <= r2)
      {
        vtkm::Float32 tch = vtkm::Sqrt(r2 - d);
        distance = dot1 - tch;
      }
    }
    //if(distance < 0) printf("bad distance");
  }
};

class Lights : public vtkm::cont::ExecutionObjectBase
{

public:
  std::vector<vtkm::Float32> Radii;
  std::vector<vtkm::Vec<vtkm::Float32, 3>> Locations;
  std::vector<vtkm::Vec<vtkm::Float32, 3>> Colors;

public:
  void AddLight(vtkm::Float32 raduis,
                vtkm::Vec<vtkm::Float32, 3> location,
                vtkm::Vec<vtkm::Float32, 3> color)
  {
    Radii.push_back(raduis);
    Locations.push_back(location);
    Colors.push_back(color);
  }

  void ClearLights()
  {
    Radii.clear();
    Locations.clear();
    Colors.clear();
  }

  template <typename Device>
  LightsExec<Device> PrepareForExecution(Device, vtkm::cont::Token& token) const
  {
    vtkm::cont::ArrayHandle<vtkm::Float32> radii =
      vtkm::cont::make_ArrayHandle(Radii, vtkm::CopyFlag::On);
    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 3>> locs =
      vtkm::cont::make_ArrayHandle(Locations, vtkm::CopyFlag::On);
    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 3>> colors =
      vtkm::cont::make_ArrayHandle(Colors, vtkm::CopyFlag::On);
    return LightsExec<Device>(radii, locs, colors, Device{}, token);
  }
};

class CheckLightContribution : public vtkm::worklet::WorkletMapField
{
protected:
  vtkm::Vec<vtkm::Float32, 3> BGColor;

public:
  VTKM_CONT
  CheckLightContribution(vtkm::Vec<vtkm::Float32, 3> bgColor)
    : BGColor(bgColor)
  {
  }
  using ControlSignature =
    void(FieldIn, FieldIn, FieldIn, WholeArrayInOut, WholeArrayInOut, ExecObject lights);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, WorkIndex);

  template <typename BufferType, typename LightType>
  VTKM_EXEC inline void operator()(const vtkm::Vec<vtkm::Float32, 3>& vtkmNotUsed(origin),
                                   const vtkm::Vec<vtkm::Float32, 3>& vtkmNotUsed(direction),
                                   const vtkm::Id& depth,
                                   BufferType& frameBuffer,
                                   BufferType& throughputBuffer,
                                   const LightType& vtkmNotUsed(lights),
                                   const vtkm::Id& pixelIndex) const
  {
    (void)depth; // TODO: remove
    vtkm::Vec<vtkm::Float32, 4> color;
    color[0] = static_cast<vtkm::Float32>(frameBuffer.Get(pixelIndex * 4 + 0));
    color[1] = static_cast<vtkm::Float32>(frameBuffer.Get(pixelIndex * 4 + 1));
    color[2] = static_cast<vtkm::Float32>(frameBuffer.Get(pixelIndex * 4 + 2));
    color[3] = static_cast<vtkm::Float32>(frameBuffer.Get(pixelIndex * 4 + 3));

    vtkm::Vec<vtkm::Float32, 3> throughput;
    throughput[0] = static_cast<vtkm::Float32>(throughputBuffer.Get(pixelIndex * 3 + 0));
    throughput[1] = static_cast<vtkm::Float32>(throughputBuffer.Get(pixelIndex * 3 + 1));
    throughput[2] = static_cast<vtkm::Float32>(throughputBuffer.Get(pixelIndex * 3 + 2));

    /*
    vtkm::Vec<vtkm::Float32, 3> contribution = BGColor;
    vtkm::Vec<vtkm::Float32, 3> lcolor(0.f, 0.f, 0.f);
    bool hitLight = lights.IntersectLights(origin, direction, lcolor);
    if (hitLight)
    {
      contribution = lcolor;
      //std::cout<<"hit "<<lcolor<<" "<<throughput<<"\n";
    }

    //vtkm::Float32 inv_pdf = 4.f * vtkm::Pif();
    color[0] += throughput[0] * contribution[0];
    color[1] += throughput[1] * contribution[1];
    color[2] += throughput[2] * contribution[2];
    color[3] = 1.f;
    */

    color[0] = throughput[0];
    color[1] = throughput[1];
    color[2] = throughput[2];
    color[3] = 1.0f;

    frameBuffer.Set(pixelIndex * 4 + 0, color[0]);
    frameBuffer.Set(pixelIndex * 4 + 1, color[1]);
    frameBuffer.Set(pixelIndex * 4 + 2, color[2]);
    frameBuffer.Set(pixelIndex * 4 + 3, color[3]);
  }
}; //class ConstantColor

} // namespace raytracing
} // namespace rendering
} // namespace vtkm
#endif
