#include <pilot/mpi/Environment.h>

#include <vtkm/Math.h>
#include <vtkm/cont/EnvironmentTracker.h>
#include <vtkm/cont/ErrorBadValue.h>

#include <limits.h>
#include <unistd.h>

namespace
{
constexpr int UNINTIALIZED_RANK = -1;
}

namespace pilot
{
namespace mpi
{
Environment::Environment()
  : Env(nullptr)
  , Comm(nullptr)
  , Shape(TopologyShape::Unknown)
  , Rank(UNINTIALIZED_RANK)
  , Size(0)
  , XLength(0)
  , YLength(0)
  , ZLength(0)
  , XRank(0)
  , YRank(0)
  , ZRank(0)
{
}

pilot::Result<bool, std::string> Environment::Initialize(int argc, char* argv[])
{
  if (this->Rank == UNINTIALIZED_RANK)
  {
    char hostname[HOST_NAME_MAX];
    gethostname(hostname, HOST_NAME_MAX);
    this->Hostname = hostname;
    this->Env = std::make_shared<vtkmdiy::mpi::environment>(argc, argv);
    this->Comm = std::make_shared<vtkmdiy::mpi::communicator>();
    vtkm::cont::EnvironmentTracker::SetCommunicator(*(this->Comm));
    this->Rank = this->Comm->rank();
    this->Size = this->Comm->size();
  }
  return pilot::Result<bool, std::string>::Success(true);
}

void Environment::ReshapeAsLine()
{
  this->Shape = TopologyShape::Line;
  this->YLength = 1;
  this->XLength = this->Size;
  this->ZLength = 1;
  this->XRank = this->Rank / this->YLength;
  this->YRank = this->Rank - (this->XRank * this->YLength);
  this->ZRank = 0;
  // Fmt::Println0("Shaping into line topology with dims: {}, {}", this->XLength, this->YLength);
}

void Environment::ReshapeAsRectangle()
{
  vtkm::Id sizeSqrt = static_cast<vtkm::Id>(vtkm::Sqrt(this->Size));
  vtkm::Id yLength = sizeSqrt;
  vtkm::Id xLength = this->Size / sizeSqrt;
  if ((xLength * yLength) != this->Size)
  {
    throw vtkm::cont::ErrorBadValue(
      fmt::format("Cannot shape {} ranks into a rectangle topology", this->Size));
  }

  this->Shape = TopologyShape::Rectangle;
  this->YLength = yLength;
  this->XLength = xLength;
  this->ZLength = 1;
  this->XRank = this->Rank / this->YLength;
  this->YRank = this->Rank - (this->XRank * this->YLength);
  this->ZRank = 0;

  // Fmt::Println0("Shaping into rectangle topology with dim: {}, {}", this->XLength, this->YLength);
}

void Environment::ReshapeAsCuboid()
{
  vtkm::Id sizeCbrt = static_cast<vtkm::Id>(vtkm::Cbrt(this->Size));
  vtkm::Id zLength = sizeCbrt;
  vtkm::Id yLength = sizeCbrt;
  vtkm::Id xLength = sizeCbrt;
  if ((xLength * yLength * zLength) != this->Size)
  {
    throw vtkm::cont::ErrorBadValue(
      fmt::format("Cannot shape {} ranks into a cube topology", this->Size));
  }

  this->Shape = TopologyShape::Cuboid;
  this->ZLength = zLength;
  this->YLength = yLength;
  this->XLength = xLength;
  vtkm::Id area = this->XLength * this->YLength;
  this->ZRank = this->Rank / area;
  this->XRank = (this->Rank % area) / this->YLength;
  this->YRank = (this->Rank % area) - (this->XRank * this->YLength);

  /*
  Fmt::Println0("Shaping into cube topology with dims: {}, {}, {}",
                this->XLength,
                this->YLength,
                this->ZLength);
  */
}

std::shared_ptr<Environment> Environment::Get()
{
  static std::shared_ptr<Environment> mpi;
  if (mpi == nullptr)
  {
    mpi = std::shared_ptr<Environment>(new Environment());
  }
  return mpi;
}
}
}
