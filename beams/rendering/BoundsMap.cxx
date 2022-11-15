#include "BoundsMap.h"

#include "../Intersections.h"
#include <vtkm/cont/AssignerPartitionedDataSet.h>
#include <vtkm/cont/EnvironmentTracker.h>
#include <vtkm/thirdparty/diy/diy.h>
#include <vtkm/thirdparty/diy/mpi-cast.h>

#include <mpi.h>

namespace beams
{
namespace rendering
{
BoundsMap::BoundsMap()
  : LocalNumBlocks(0)
  , TotalNumBlocks(0)
{
}

BoundsMap::BoundsMap(const vtkm::cont::DataSet& dataSet)
  : LocalNumBlocks(1)
  , TotalNumBlocks(0)
{
  this->Init({ dataSet });
}

BoundsMap::BoundsMap(const std::vector<vtkm::cont::DataSet>& dataSets)
  : LocalNumBlocks(static_cast<vtkm::Id>(dataSets.size()))
  , TotalNumBlocks(0)
{
  this->Init(dataSets);
}

BoundsMap::BoundsMap(const vtkm::cont::PartitionedDataSet& pds)
  : LocalNumBlocks(pds.GetNumberOfPartitions())
  , TotalNumBlocks(0)
{
  this->Init(pds.GetPartitions());
}

vtkm::Id BoundsMap::GetLocalBlockId() const
{
  return this->LocalID;
}

int BoundsMap::FindRank(vtkm::Id blockId) const
{
  auto it = this->BlockToRankMap.find(blockId);
  if (it == this->BlockToRankMap.end())
    return -1;
  return it->second;
}

vtkm::Id BoundsMap::FindBlock(const vtkm::Vec3f& p) const
{
  return this->FindBlock(p, -1);
}

vtkm::Id BoundsMap::FindBlock(const vtkm::Vec3f& p, vtkm::Id ignoreBlock) const
{
  std::vector<vtkm::Id> blockIDs;
  if (this->GlobalBounds.Contains(p))
  {
    vtkm::Id blockId = 0;
    for (auto& it : this->BlockBounds)
    {
      if (blockId != ignoreBlock && it.Contains(p))
        blockIDs.push_back(blockId);
      blockId++;
    }
  }

  return (blockIDs.size() > 0 ? blockIDs[0] : -1);
}

void BoundsMap::Init(const std::vector<vtkm::cont::DataSet>& dataSets)
{
  vtkm::cont::AssignerPartitionedDataSet assigner(this->LocalNumBlocks);
  this->TotalNumBlocks = assigner.nblocks();
  std::vector<int> ids;

  vtkmdiy::mpi::communicator Comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
  assigner.local_gids(Comm.rank(), ids);
  for (const auto& i : ids)
    this->LocalID = static_cast<vtkm::Id>(i);

  for (vtkm::Id id = 0; id < this->TotalNumBlocks; id++)
    this->BlockToRankMap[id] = assigner.rank(static_cast<int>(id));
  this->Build(dataSets);
}

void BoundsMap::Build(const std::vector<vtkm::cont::DataSet>& dataSets)
{
  std::vector<vtkm::Float64> vals(static_cast<std::size_t>(this->TotalNumBlocks * 6), 0);
  std::vector<vtkm::Float64> vals2(vals.size());

  const vtkm::cont::DataSet& ds = dataSets[0];
  vtkm::Bounds bounds = ds.GetCoordinateSystem().GetBounds();

  std::size_t idx = static_cast<std::size_t>(this->LocalID * 6);
  vals[idx++] = bounds.X.Min;
  vals[idx++] = bounds.X.Max;
  vals[idx++] = bounds.Y.Min;
  vals[idx++] = bounds.Y.Max;
  vals[idx++] = bounds.Z.Min;
  vals[idx++] = bounds.Z.Max;

  vtkmdiy::mpi::communicator Comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
  MPI_Comm mpiComm = vtkmdiy::mpi::mpi_cast(Comm.handle());
  MPI_Allreduce(vals.data(), vals2.data(), vals.size(), MPI_DOUBLE, MPI_SUM, mpiComm);

  this->BlockBounds.resize(static_cast<std::size_t>(this->TotalNumBlocks));
  this->GlobalBounds = vtkm::Bounds();
  idx = 0;
  for (auto& block : this->BlockBounds)
  {
    block = vtkm::Bounds(vals2[idx + 0],
                         vals2[idx + 1],
                         vals2[idx + 2],
                         vals2[idx + 3],
                         vals2[idx + 4],
                         vals2[idx + 5]);
    this->GlobalBounds.Include(block);
    idx += 6;
  }
}
} // namespace rendering
} // namespace beams