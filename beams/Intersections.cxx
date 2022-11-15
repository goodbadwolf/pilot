#include "Intersections.h"
#include <vtkm/VectorAnalysis.h>

namespace beams
{
namespace Intersections
{
VTKM_EXEC_CONT
bool SegmentAABB(const vtkm::Vec3f_32& start,
                 const vtkm::Vec3f_32& end,
                 vtkm::Bounds aabb,
                 vtkm::Float32& tMin,
                 vtkm::Float32& tMax,
                 vtkm::Float32 tEps)
{
  vtkm::Vec3f_32 direction = end - start;
  vtkm::Float32 segmentLength = vtkm::Magnitude(direction);
  direction = direction / segmentLength;

  tMin = 0.0f;
  tMax = vtkm::Infinity32();

  vtkm::Float32 txMin = (aabb.X.Min - start[0]) / direction[0];
  vtkm::Float32 txMax = (aabb.X.Max - start[0]) / direction[0];
  tMin = vtkm::Max(tMin, vtkm::Min(txMin, txMax));
  tMax = vtkm::Min(tMax, vtkm::Max(txMin, txMax));

  vtkm::Float32 tyMin = (aabb.Y.Min - start[1]) / direction[1];
  vtkm::Float32 tyMax = (aabb.Y.Max - start[1]) / direction[1];
  tMin = vtkm::Max(tMin, vtkm::Min(tyMin, tyMax));
  tMax = vtkm::Min(tMax, vtkm::Max(tyMin, tyMax));

  vtkm::Float32 tzMin = (aabb.Z.Min - start[2]) / direction[2];
  vtkm::Float32 tzMax = (aabb.Z.Max - start[2]) / direction[2];
  tMin = vtkm::Max(tMin, vtkm::Min(tzMin, tzMax));
  tMax = vtkm::Min(tMax, vtkm::Max(tzMin, tzMax));

  if (tMin < 0.0f)
  {
    tMin = tEps;
  }

  if (tMax > segmentLength)
  {
    tMax = segmentLength;
  }

  return tMin <= tMax;
}
}
}
