#ifndef beams_mpi_MpiEnv_h
#define beams_mpi_MpiEnv_h

#include "Result.h"

#include <vtkm/thirdparty/diy/diy.h>

#include <memory>

namespace beams
{
namespace mpi
{
struct MpiEnv
{
private:
  MpiEnv();

public:
  enum class TopologyShape
  {
    Unknown,
    Line,
    Rectangle,
    Cuboid,
  };

  static std::shared_ptr<MpiEnv> Get()
  {
    static std::shared_ptr<MpiEnv> mpi;
    if (mpi == nullptr)
    {
      mpi = std::shared_ptr<MpiEnv>(new MpiEnv());
    }
    return mpi;
  }

  beams::Result Initialize(int argc, char* argv[]);

  void ReshapeAsLine();

  void ReshapeAsRectangle();

  void ReshapeAsCuboid();

  std::shared_ptr<vtkmdiy::mpi::environment> Env;
  std::shared_ptr<vtkmdiy::mpi::communicator> Comm;

  TopologyShape Shape;
  std::string Hostname;
  int Rank;
  int Size;
  int XLength;
  int YLength;
  int ZLength;
  int XRank;
  int YRank;
  int ZRank;
};
}
} // beams::mpi

#endif // beams_mpi_env_h