#include "Beams.h"
#include "Result.h"

#include <toml++/toml.h>

#include <pilot/Logger.h>
#include <pilot/Result.h>
#include <pilot/mpi/Environment.h>
#include <pilot/staging/Stage.h>
#include <pilot/system/SystemUtils.h>

pilot::Result<pilot::mpi::EnvironmentPtr, std::string> InitializeMpi(int& argc, char** argv)
{
  using Result = pilot::Result<pilot::mpi::EnvironmentPtr, std::string>;
  auto mpi = pilot::mpi::Environment::Get();
  auto result = mpi->Initialize(argc, argv);
  if (result.IsSuccess())
  {
    return Result::Success(mpi);
  }
  else
  {
    return Result::Fail("Failed to initialize MPI");
  }
}

int main(int argc, char* argv[])
{
  auto mpiInitResult = InitializeMpi(argc, argv);
  if (mpiInitResult.IsFailure())
  {
    pilot::system::Die(fmt::format("Beams exiting: {}", mpiInitResult.Error));
  }

  beams::Beams app(mpiInitResult.Value);
  auto beamsInitResult = app.Initialize(argc, argv);
  if (!beamsInitResult.IsSuccess())
  {
    pilot::system::Die(fmt::format("Beams failed to initialize: {}", beamsInitResult.Error));
  }

  app.Run();

  return EXIT_SUCCESS;

  /*
  auto tbl = toml::parse_file("stage.toml");
  pilot::staging::Stage stage;
  auto result = stage.Deserialize(tbl);
  if (result.IsFailure())
  {
    std::cerr << result.Error << "\n";
  }
  for (auto j = stage.DataSetProviderDescriptors.cbegin();
       j != stage.DataSetProviderDescriptors.cend();
       ++j)
  {
    auto d = *j;
    std::cerr << d.second.get()->Name << "\n";
  }
  std::cerr << "Done\n";
  */
  //d.Deserialize(*(provs[0].as_table()));
  //std::cerr << provs << "\n";
  return 0;
}