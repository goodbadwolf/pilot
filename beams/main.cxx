#include "Beams.h"
#include "Result.h"

#include <pilot/Logger.h>
#include <pilot/Result.h>
#include <pilot/mpi/Environment.h>

pilot::Result<std::shared_ptr<pilot::mpi::Environment>, std::string> InitializeMpi(int& argc,
                                                                                   char** argv)
{
  auto mpi = pilot::mpi::Environment::Get();
  auto result = mpi->Initialize(argc, argv);
  if (result.IsValid() && result.Outcome)
  {
    return mpi;
  }
  else
  {
    return pilot::Result<std::shared_ptr<pilot::mpi::Environment>, std::string>(
      std::shared_ptr<pilot::mpi::Environment>());
  }
}

int main(int argc, char* argv[])
{
  auto mpiInitResult = InitializeMpi(argc, argv);
  if (mpiInitResult.IsError())
  {
    return EXIT_FAILURE;
  }
  auto mpiEnv = mpiInitResult.Outcome;
  std::cerr << "Running on host '" << mpiEnv->Hostname << "'\n";
  beams::Beams app(mpiEnv);

  auto result = app.Initialize(argc, argv);
  if (!result.Success)
  {
    if (mpiEnv->Rank == 0)
    {
      std::cerr << "Beams failed with error: " << result.Err << "\n";
    }
    return EXIT_FAILURE;
  }

  app.Run();

  return EXIT_SUCCESS;
}