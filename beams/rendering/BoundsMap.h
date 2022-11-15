#ifndef beams_rendering_boundsmap_h
#define beams_rendering_boundsmap_h

#include "../Intersections.h"
#include <vector>
#include <vtkm/Bounds.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/PartitionedDataSet.h>
#include <vtkm/rendering/vtkm_rendering_export.h>

namespace beams
{
namespace rendering
{
template <typename Device>
struct VTKM_RENDERING_EXPORT BoundsMapExec
{
  using IdHandle = vtkm::cont::ArrayHandle<vtkm::Id>;
  using BoundsHandle = vtkm::cont::ArrayHandle<vtkm::Bounds>;

  using IdPortal = typename IdHandle::ReadPortalType;
  using BoundsPortal = typename BoundsHandle::ReadPortalType;

  VTKM_CONT BoundsMapExec(const vtkm::Id& localId,
                          const vtkm::Id& totalNumBlocks,
                          const vtkm::Bounds& globalBounds,
                          const IdHandle& blockIds,
                          const IdHandle& ranks,
                          const BoundsHandle& bounds,
                          vtkm::cont::Token& token);

  VTKM_EXEC vtkm::Id FindRank(const vtkm::Vec3f_32& point, vtkm::Id ignoreBlock = -1) const;

  VTKM_EXEC bool FindSegmentBlockIntersections(const vtkm::Id& blockId,
                                               const vtkm::Vec3f_32& start,
                                               const vtkm::Vec3f_32& end,
                                               vtkm::Float32& tMin,
                                               vtkm::Float32& tMax,
                                               vtkm::Float32 tEps = 1e-4f) const;

  VTKM_EXEC vtkm::Id FindNumSegmentBlockIntersections(const vtkm::Vec3f_32& start,
                                                      const vtkm::Vec3f_32& end,
                                                      bool useGlancingHit,
                                                      vtkm::Id ignoreBlock = -1,
                                                      vtkm::Float32 tEps = 1e-4f) const;

  vtkm::Id LocalId;
  vtkm::Id TotalNumBlocks;
  vtkm::Bounds GlobalBounds;
  IdPortal BlockIds;
  IdPortal Ranks;
  BoundsPortal Bounds;
};

template <typename Device>
BoundsMapExec<Device>::BoundsMapExec(const vtkm::Id& localId,
                                     const vtkm::Id& totalNumBlocks,
                                     const vtkm::Bounds& globalBounds,
                                     const IdHandle& blockIds,
                                     const IdHandle& ranks,
                                     const BoundsHandle& bounds,
                                     vtkm::cont::Token& token)
  : LocalId(localId)
  , TotalNumBlocks(totalNumBlocks)
  , GlobalBounds(globalBounds)
  , BlockIds(blockIds.PrepareForInput(Device(), token))
  , Ranks(ranks.PrepareForInput(Device(), token))
  , Bounds(bounds.PrepareForInput(Device(), token))
{
}

template <typename Device>
vtkm::Id BoundsMapExec<Device>::FindRank(const vtkm::Vec3f_32& point, vtkm::Id ignoreBlock) const
{
  vtkm::Id foundBlock = -1;
  if (this->GlobalBounds.Contains(point))
  {
    for (vtkm::Id i = 0; i < this->Bounds.GetNumberOfValues(); ++i)
    {
      const vtkm::Bounds& b = this->Bounds.Get(i);
      if (i != ignoreBlock && b.Contains(point))
      {
        foundBlock = i;
      }
    }
  }

  return foundBlock;
}

template <typename Device>
bool BoundsMapExec<Device>::FindSegmentBlockIntersections(const vtkm::Id& blockId,
                                                          const vtkm::Vec3f_32& start,
                                                          const vtkm::Vec3f_32& end,
                                                          vtkm::Float32& tMin,
                                                          vtkm::Float32& tMax,
                                                          vtkm::Float32 tEps) const
{
  const vtkm::Bounds& blockBounds = this->Bounds.Get(blockId);
  bool hits = beams::Intersections::SegmentAABB(start, end, blockBounds, tMin, tMax, tEps);
  return hits;
}

template <typename Device>
vtkm::Id BoundsMapExec<Device>::FindNumSegmentBlockIntersections(const vtkm::Vec3f_32& start,
                                                                 const vtkm::Vec3f_32& end,
                                                                 bool useGlancingHit,
                                                                 vtkm::Id ignoreBlock,
                                                                 vtkm::Float32 tEps) const
{
  vtkm::Id numHits = 0;
  for (vtkm::Id blockId = 0; blockId < this->TotalNumBlocks; ++blockId)
  {
    if (blockId == ignoreBlock)
    {
      continue;
    }
    vtkm::Float32 tMin, tMax;
    bool hits = this->FindSegmentBlockIntersections(blockId, start, end, tMin, tMax, tEps);
    bool isGlancingHit = beams::Intersections::ApproxEquals(tMin, tMax);
    if (hits && (useGlancingHit || !isGlancingHit))
    {
      numHits++;
    }
  }
  return numHits;
}

struct VTKM_RENDERING_EXPORT BoundsMap : public vtkm::cont::ExecutionObjectBase
{
public:
  using IdHandle = vtkm::cont::ArrayHandle<vtkm::Id>;
  using BoundsHandle = vtkm::cont::ArrayHandle<vtkm::Bounds>;

  using IdPortal = typename IdHandle::ReadPortalType;
  using BoundsPortal = typename BoundsHandle::ReadPortalType;

  BoundsMap();

  BoundsMap(const vtkm::cont::DataSet& dataSet);

  BoundsMap(const std::vector<vtkm::cont::DataSet>& dataSets);

  BoundsMap(const vtkm::cont::PartitionedDataSet& pds);

  vtkm::Id GetLocalBlockId() const;

  int FindRank(vtkm::Id blockId) const;

  vtkm::Id FindBlock(const vtkm::Vec3f& p) const;

  vtkm::Id FindBlock(const vtkm::Vec3f& p, vtkm::Id ignoreBlock) const;

  template <typename Device>
  BoundsMapExec<Device> PrepareForExecution(Device, vtkm::cont::Token& token) const
  {
    IdHandle blockIds;
    IdHandle ranks;
    BoundsHandle bounds;
    blockIds.Allocate(this->TotalNumBlocks);
    ranks.Allocate(this->TotalNumBlocks);
    bounds.Allocate(this->TotalNumBlocks);

    IdHandle::WritePortalType blockIdsPortal = blockIds.WritePortal();
    IdHandle::WritePortalType ranksPortal = ranks.WritePortal();
    BoundsHandle::WritePortalType boundsPortal = bounds.WritePortal();
    for (vtkm::Id rank = 0; rank < this->TotalNumBlocks; ++rank)
    {
      auto blockId = rank;
      auto bound = this->BlockBounds[rank];
      blockIdsPortal.Set(rank, blockId);
      ranksPortal.Set(rank, rank);
      boundsPortal.Set(rank, bound);
    }

    return BoundsMapExec<Device>(
      this->LocalID, this->TotalNumBlocks, this->GlobalBounds, blockIds, ranks, bounds, token);
  }
  vtkm::Id LocalID;
  vtkm::Id LocalNumBlocks;
  vtkm::Id TotalNumBlocks;
  std::map<vtkm::Id, vtkm::Int32> BlockToRankMap;
  std::vector<vtkm::Bounds> BlockBounds;
  vtkm::Bounds GlobalBounds;

private:
  void Init(const std::vector<vtkm::cont::DataSet>& dataSets);

  void Build(const std::vector<vtkm::cont::DataSet>& dataSets);
};

} // namespace rendering
} // namespace beams

#endif // beams_rendering_boundsmap_h