#ifndef beams_beams_h
#define beams_beams_h

#include "Result.h"

#include <pilot/mpi/Environment.h>

#include <memory>
#include <string>

namespace beams
{
class Beams
{
public:
  Beams(std::shared_ptr<pilot::mpi::Environment> mpiEnv);

  beams::Result Initialize(int& argc, char** argv);

  void Run();

  struct InternalsType;

private:
  void LoadScene();

  void SetupScene();

  void SaveCanvas(int num);

  void SaveDataSet(int num);

  std::shared_ptr<InternalsType> Internals;
}; // class Beams

} // namespace beams

#endif