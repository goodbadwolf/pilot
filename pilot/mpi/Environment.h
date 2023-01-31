#pragma once

#include <pilot/Result.h>
#include <pilot/mpi/TopologyShape.h>

#include <vtkm/thirdparty/diy/diy.h>

#include <memory>
#include <string>

namespace pilot
{
namespace mpi
{
struct Environment
{
public:
  static std::shared_ptr<Environment> Get();

  pilot::Result<bool, std::string> Initialize(int argc, char* argv[]);

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

private:
  Environment();
};

using EnvironmentPtr = std::shared_ptr<Environment>;
}
}