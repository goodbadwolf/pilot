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
#ifndef vtk_m_rendering_raytracing_RectilinearMeshOracle_h
#define vtk_m_rendering_raytracing_RectilinearMeshOracle_h

#include <vtkm/cont/CellSetStructured.h>

#include <memory>

namespace vtkm
{
namespace rendering
{
namespace raytracing
{

template <typename Device>
class RectilinearMeshOracleExecObj
{
protected:
  using DefaultHandle = vtkm::cont::ArrayHandle<vtkm::FloatDefault>;
  using CartesianArrayHandle =
    vtkm::cont::ArrayHandleCartesianProduct<DefaultHandle, DefaultHandle, DefaultHandle>;
  using DefaultConstHandle = typename DefaultHandle::ReadPortalType;
  using CartesianConstPortal = typename CartesianArrayHandle::ReadPortalType;

  DefaultConstHandle CoordPortals[3];
  CartesianConstPortal Coordinates;
  vtkm::exec::ConnectivityStructured<vtkm::TopologyElementTagPoint, vtkm::TopologyElementTagCell, 3>
    Conn;

  vtkm::Vec<vtkm::Float32, 3> MinPoint;
  vtkm::Vec<vtkm::Float32, 3> MaxPoint;
  vtkm::Id3 PointDims;
  vtkm::Id3 CellDims;

public:
  VTKM_CONT
  RectilinearMeshOracleExecObj(const vtkm::cont::CellSetStructured<3>& cellset,
                               const CartesianArrayHandle& coordinates,
                               Device,
                               vtkm::cont::Token& token)
    : Coordinates(coordinates.PrepareForInput(Device(), token))
    , Conn(cellset.PrepareForInput(Device(),
                                   vtkm::TopologyElementTagPoint(),
                                   vtkm::TopologyElementTagCell(),
                                   token))
  {
    CoordPortals[0] = Coordinates.GetFirstPortal();
    CoordPortals[1] = Coordinates.GetSecondPortal();
    CoordPortals[2] = Coordinates.GetThirdPortal();
    PointDims = Conn.GetPointDimensions();

    CellDims[0] = PointDims[0] - 1;
    CellDims[1] = PointDims[1] - 1;
    CellDims[2] = PointDims[2] - 1;

    MinPoint[0] = static_cast<vtkm::Float32>(coordinates.ReadPortal().GetFirstPortal().Get(0));
    MinPoint[1] = static_cast<vtkm::Float32>(coordinates.ReadPortal().GetSecondPortal().Get(0));
    MinPoint[2] = static_cast<vtkm::Float32>(coordinates.ReadPortal().GetThirdPortal().Get(0));

    MaxPoint[0] =
      static_cast<vtkm::Float32>(coordinates.ReadPortal().GetFirstPortal().Get(PointDims[0] - 1));
    MaxPoint[1] =
      static_cast<vtkm::Float32>(coordinates.ReadPortal().GetSecondPortal().Get(PointDims[1] - 1));
    MaxPoint[2] =
      static_cast<vtkm::Float32>(coordinates.ReadPortal().GetThirdPortal().Get(PointDims[2] - 1));
  }

  template <typename T>
  VTKM_EXEC inline bool IsInside(const vtkm::Vec<T, 3>& point) const
  {
    bool inside = true;
    if (point[0] < MinPoint[0] || point[0] > MaxPoint[0])
      inside = false;
    if (point[1] < MinPoint[1] || point[1] > MaxPoint[1])
      inside = false;
    if (point[2] < MinPoint[2] || point[2] > MaxPoint[2])
      inside = false;
    return inside;
  }

  VTKM_EXEC_CONT
  vtkm::Float32 Interpolate(const vtkm::Vec<vtkm::Float32, 8>& scalars,
                            const vtkm::Vec<vtkm::Float32, 3>& pcoords) const
  {
    vtkm::Float32 scalar;
    vtkm::exec::CellInterpolate(scalars, pcoords, vtkm::CellShapeTagHexahedron(), scalar);
    return scalar;
  }

  VTKM_EXEC
  inline vtkm::Id GetCellIndex(const vtkm::Vec<vtkm::Id, 3>& cell) const
  {
    return (cell[2] * (CellDims[1]) + cell[1]) * (CellDims[0]) + cell[0];
  }

  VTKM_EXEC_CONT
  vtkm::Int32 GetCellIndices(vtkm::Id cellIndices[8], const vtkm::Id& cellIndex) const
  {
    vtkm::Id3 cellId;
    cellId[0] = cellIndex % CellDims[0];
    cellId[1] = (cellIndex / CellDims[0]) % CellDims[1];
    cellId[2] = cellIndex / (CellDims[0] * CellDims[1]);
    cellIndices[0] = (cellId[2] * PointDims[1] + cellId[1]) * PointDims[0] + cellId[0];
    cellIndices[1] = cellIndices[0] + 1;
    cellIndices[2] = cellIndices[1] + PointDims[0];
    cellIndices[3] = cellIndices[2] - 1;
    cellIndices[4] = cellIndices[0] + PointDims[0] * PointDims[1];
    cellIndices[5] = cellIndices[4] + 1;
    cellIndices[6] = cellIndices[5] + PointDims[0];
    cellIndices[7] = cellIndices[6] - 1;
    return 8;
  }

  VTKM_EXEC_CONT
  vtkm::UInt8 GetCellShape(const vtkm::Id& vtkmNotUsed(cellId)) const
  {
    return vtkm::UInt8(CELL_SHAPE_HEXAHEDRON);
  }

  template <typename T>
  VTKM_EXEC_CONT void FindCellImpl(const vtkm::Vec<T, 3>& point,
                                   vtkm::Id& cellId,
                                   vtkm::Vec<T, 3>& pcoords) const
  {
    // check is in -> search assumes this
    vtkm::Vec<vtkm::Id, 3> cell;
    cellId = -1;
    bool inside = IsInside(point);

    if (!inside)
    {
      return;
    }

    for (vtkm::Int32 dim = 0; dim < 3; ++dim)
    {
      vtkm::Id low = 0;
      vtkm::Id hi = CellDims[dim] - 1;
      const T val = point[dim];

      while (low <= hi)
      {
        vtkm::Id mid = (low + hi) / 2;
        T pmid = static_cast<T>(CoordPortals[dim].Get(mid + 1));

        if (val <= pmid)
        {
          hi = mid - 1;
        }
        else
        {
          low = mid + 1;
        }
      }

      cell[dim] = low;
    }


    vtkm::Vec<T, 3> minPoint;
    vtkm::Vec<T, 3> maxPoint;

    for (vtkm::Int32 i = 0; i < 3; ++i)
    {
      minPoint[i] = static_cast<T>(CoordPortals[i].Get(cell[i]));
      vtkm::Id j = cell[i] + 1;
      j = j >= CoordPortals[i].GetNumberOfValues() ? cell[i] : j;
      maxPoint[i] = static_cast<T>(CoordPortals[i].Get(j));
    }
    vtkm::VecAxisAlignedPointCoordinates<3> rPoints(minPoint, maxPoint - minPoint);

    vtkm::ErrorCode result = vtkm::exec::WorldCoordinatesToParametricCoordinates(
      rPoints, point, vtkm::CellShapeTagHexahedron(), pcoords);
    VTKM_ASSERT(result == vtkm::ErrorCode::Success);
    cellId = GetCellIndex(cell);
  }

  VTKM_EXEC_CONT
  void FindCell(const vtkm::Vec<vtkm::Float32, 3>& point,
                vtkm::Id& cellId,
                vtkm::Vec<Float32, 3>& pcoords) const
  {
    FindCellImpl(point, cellId, pcoords);
  }

  VTKM_EXEC_CONT
  void FindCell(const vtkm::Vec<vtkm::Float64, 3>& point,
                vtkm::Id& cellId,
                vtkm::Vec<Float64, 3>& pcoords) const
  {
    FindCellImpl(point, cellId, pcoords);
  }
};

class RectilinearMeshOracle : public vtkm::cont::ExecutionObjectBase
{
public:
  using DefaultHandle = vtkm::cont::ArrayHandle<vtkm::FloatDefault>;
  using CartesianArrayHandle =
    vtkm::cont::ArrayHandleCartesianProduct<DefaultHandle, DefaultHandle, DefaultHandle>;

  VTKM_CONT
  RectilinearMeshOracle(const vtkm::cont::CellSetStructured<3>& cellSet,
                        const CartesianArrayHandle& coordinates)
    : CellSet(cellSet)
    , Coordinates(coordinates)
  {
  }

  template <typename Device>
  VTKM_CONT RectilinearMeshOracleExecObj<Device> PrepareForExecution(Device,
                                                                     vtkm::cont::Token& token) const
  {
    return RectilinearMeshOracleExecObj<Device>(this->CellSet, this->Coordinates, Device{}, token);
  }

private:
  vtkm::cont::CellSetStructured<3> CellSet;
  CartesianArrayHandle Coordinates;
};

}
}
} //namespace vtkm::rendering::raytracing
#endif
