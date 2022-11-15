//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include "Beams.h"
#include "Result.h"
#include "utils/Fmt.h"

std::shared_ptr<beams::mpi::MpiEnv> InitializeMpi(int& argc, char** argv)
{
  auto mpi = beams::mpi::MpiEnv::Get();
  beams::Result result = mpi->Initialize(argc, argv);
  if (result.Success)
  {
    return mpi;
  }
  else
  {
    return nullptr;
  }
}

int main(int argc, char* argv[])
{
  std::shared_ptr<beams::mpi::MpiEnv> mpiEnv = InitializeMpi(argc, argv);
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