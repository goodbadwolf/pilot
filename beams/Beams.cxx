#include "Beams.h"
#include "Config.h"
#include "SceneFactory.h"
// #include "Window.h"
#include "Profiler.h"
#include "rendering/BoundsMap.h"
#include "rendering/Scene.h"
#include "utils/Json.h"
#include <pilot/Logger.h>

#include <pilot/io/DataSetUtils.h>
#include <pilot/io/FileSystemUtils.h>
#include <pilot/mpi/Environment.h>
#include <pilot/system/SystemUtils.h>

#include <vtkm/cont/Initialize.h>
#include <vtkm/io/FileUtils.h>
#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/thirdparty/diy/diy.h>
#include <vtkm/thirdparty/diy/mpi-cast.h>

#include <fmt/core.h>
#include <toml++/toml.h>

#include <fstream>
#include <iomanip>
#include <map>
#include <mpi.h>

vtkm::Float64 Phase1Time;
vtkm::Float64 Phase2Time;
vtkm::Float64 Phase3Time;
vtkm::Float64 Phase4Time;
vtkm::Float64 Phase5Time;

namespace beams
{
#define CHECK_RESULT(statement)                       \
  {                                                   \
    auto result = (statement);                        \
    if (!result.IsSuccess())                          \
    {                                                 \
      return pilot::Result<bool>::Fail(result.Error); \
    }                                                 \
  }


struct Beams::InternalsType
{
  InternalsType() = default;

  pilot::Result<bool> ParseArgs(int argc, char** argv)
  {
    // argv[0] is the name of the binary, so skipping
    for (int i = 1; i < argc; ++i)
    {
      auto arg = std::string(argv[i]);
      auto seperatorPos = arg.find("=");
      if (seperatorPos == std::string::npos)
      {
        continue;
      }

      auto argKey = arg.substr(0, seperatorPos);
      auto argVal = arg.substr(seperatorPos + 1);
      if (argKey == "--base-dir")
      {
        this->BaseDir = argVal;
      }
      else if (argKey == "--preset")
      {
        this->StartPresetName = argVal;
      }
    }

    if (this->BaseDir.length() > 0)
    {
      return pilot::Result<bool>::Success(true);
    }
    else
    {
      return pilot::Result<bool>::Fail("--base-dir not specified");
    }
  }

  pilot::BoolResult LoadConfig()
  {
    using Result = pilot::BoolResult;

    const std::string CONFIG_FILE_NAME = "stage.toml";
    std::string configFilePath = vtkm::io::MergePaths(this->BaseDir, CONFIG_FILE_NAME);
    if (!pilot::io::FileSystemUtils::FileExists(configFilePath))
    {
      return Result::Fail(fmt::format("Config file '{}' not found", configFilePath));
    }

    toml::table configTbl = toml::parse_file(configFilePath);
    auto configLoadResult = this->Config.Deserialize(configTbl);
    if (!configLoadResult.IsSuccess())
    {
      return Result::Fail(configLoadResult.Error);
    }

    return Result::Success(true);
  }

  /*
  Result LoadScene(const std::string& presetName)
  {
    this->CurrentPresetName = presetName;
    return this->LoadDataSet();
  }

  Result LoadDataSet()
  {
    const beams::Preset& preset = this->Cfg.Presets[this->CurrentPresetName];
    this->Scene = SceneFactory::CreateFromPreset(preset);

    if (this->Scene->DataSet.GetNumberOfFields() > 0)
    {
      return Result::Succeeded();
    }
    else
    {
      return Result::Failed(
        fmt::format("Unable to load data set for preset '{}'", this->CurrentPresetName));
    }
  }
  */

  std::string BaseDir;
  std::string StartPresetName;
  // beams::Window Win;
  beams::Config Config;
  std::string CurrentPresetName;
  std::shared_ptr<beams::rendering::Scene> Scene;
  std::shared_ptr<vtkm::rendering::CanvasRayTracer> Canvas;
  std::shared_ptr<pilot::mpi::Environment> Mpi;
  std::shared_ptr<beams::Profiler> Profiler;
};

int ToMs(vtkm::Float64 t)
{
  return static_cast<int>(t * 1000);
}

void PrintTimeStats(const std::string& label, vtkm::Float64 time)
{
  auto mpi = pilot::mpi::Environment::Get();
  auto comm = mpi->Comm;
  MPI_Comm mpiComm = vtkmdiy::mpi::mpi_cast(comm->handle());
  std::vector<vtkm::Float64> allTimes(mpi->Size, -1.0f);
  MPI_Gather(&time, 1, MPI_DOUBLE, allTimes.data(), 1, MPI_DOUBLE, 0, mpiComm);
  if (mpi->Rank == 0)
  {
    auto minMaxTime = std::minmax_element(allTimes.begin(), allTimes.end());
    auto minTime = ToMs(*(minMaxTime.first));
    auto maxTime = ToMs(*(minMaxTime.second));
    auto avgTime = ToMs(std::accumulate(allTimes.begin(), allTimes.end(), 0.0) /
                        static_cast<vtkm::Float64>(allTimes.size()));
    LOG::Println("{}: Min = {} ms, Max = {} ms, Avg = {} ms", label, minTime, maxTime, avgTime);
  }
}

void PrintRootTimeStat(const std::string& label, vtkm::Float64 time)
{
  auto mpi = pilot::mpi::Environment::Get();
  if (mpi->Rank == 0)
  {
    LOG::Println("{}: Time = {} ms", label, ToMs(time));
  }
}

Beams::Beams(std::shared_ptr<pilot::mpi::Environment> mpiEnv)
  : Internals(new InternalsType())
{
  this->Internals->Mpi = mpiEnv;
}

pilot::Result<bool> Beams::Initialize(int& argc, char** argv)
{
  auto mpiEnv = this->Internals->Mpi;
  LOG::Info("Running Beams on host '{}'", mpiEnv->Hostname);

  auto opts = vtkm::cont::InitializeOptions::RequireDevice;
  vtkm::cont::Initialize(argc, argv, opts);
  pilot::system::PrintVtkmDeviceInfo();

  CHECK_RESULT(this->Internals->ParseArgs(argc, argv));
  CHECK_RESULT(this->Internals->LoadConfig());
  /*
  result = this->Internals->Win.Initialize("Beams", 1920, 1080);
  CHECK_RESULT(result)
  */
  return pilot::Result<bool>::Success(true);
}

void Beams::LoadScene()
{
  /*
  std::string presetName;
  if (!(this->Internals->StartPresetName.empty()))
  {
    presetName = this->Internals->StartPresetName;
  }
  else
  {
    presetName = this->Internals->Cfg.DefaultPresetName;
  }
  this->Internals->LoadScene(presetName);

  if (!this->Internals->Canvas)
  {
    const vtkm::Id CSIZE = 1024;
    this->Internals->Canvas = std::make_shared<vtkm::rendering::CanvasRayTracer>(CSIZE, CSIZE);
  }
  this->Internals->Scene->Canvas = this->Internals->Canvas.get();

  auto scene = this->Internals->Scene;
  auto mpi = pilot::mpi::Environment::Get();
  LOG::Println0("Scene loaded with global bounds: {}", scene->BoundsMap->GlobalBounds);
  */
}

void Beams::SetupScene()
{
  /*
  this->Internals->Canvas->Clear();
  this->Internals->Scene->Ready();
  */
}

void Beams::Run()
{
  /*
  auto mpi = pilot::mpi::Environment::Get();

  this->LoadScene();
  vtkm::Vec3f_32 lightPosOrigin{ 0.5f, 0.5f, 0.5f };
  vtkm::Float32 r = 0.5f;
  vtkm::Float32 theta = 0.0f;
  vtkm::Float32 phi = 0.0f;
  vtkm::Float32 maxPhi = vtkm::TwoPif();
  vtkm::Float32 maxTheta = vtkm::Pif();
  vtkm::Float32 dPhi = 5.0f * vtkm::Pi_180f();
  // vtkm::Float32 dThetha = 5.0f * vtkm::Pi_180f();

  std::shared_ptr<beams::rendering::Scene> scene = this->Internals->Scene;
  int count = 0;
  while (theta <= maxTheta && phi <= maxPhi)
  {
    // scene->LightPosition =
    //   lightPosOrigin + r * vtkm::Vec3f_32{ 0.0f, vtkm::Sin(phi), vtkm::Cos(phi) };
    scene->LightPosition =
      lightPosOrigin + r * vtkm::Vec3f_32{ vtkm::Cos(phi), vtkm::Sin(phi), 0.0f };
    // scene->LightPosition = vtkm::Vec3f_32{ 0.0f, 0.5f, 1.5f };
    this->SetupScene();
    this->Internals->Profiler = std::make_shared<beams::Profiler>("RenderCells");
    scene->Mapper.SetProfiler(this->Internals->Profiler);
    scene->Mapper.RenderCells(scene->DataSet.GetCellSet(),
                              scene->DataSet.GetCoordinateSystem(),
                              scene->DataSet.GetField(scene->FieldName),
                              scene->ColorTable,
                              scene->Camera,
                              scene->Range);
    this->Internals->Profiler->Collect();
    if (mpi->Rank == 0)
    {
      std::stringstream ss;
      ss << "\n";
      this->Internals->Profiler->PrintSummary(ss);
      LOG::Println(ss.str());
    }
    PrintTimeStats("Phase 1", Phase1Time);
    PrintTimeStats("Phase 2", Phase2Time);
    PrintTimeStats("Phase 3", Phase3Time);
    PrintTimeStats("Phase 4", Phase4Time);
    PrintRootTimeStat("Phase 5", Phase5Time);

    if (mpi->Rank == 0)
    {
      this->SaveCanvas(count++);
      // this->SaveDataSet(0);
    }

    // theta += dThetha;
    phi += dPhi;
  }
  */
}

void Beams::SaveCanvas(int num)
{
  auto mpi = pilot::mpi::Environment::Get();
  std::stringstream ss;
  std::stringstream numSs;
  numSs << std::setw(3) << std::setfill('0') << num;
  ss << this->Internals->Scene->Id << "_" << mpi->Rank << "_" << numSs.str() << ".png";
  LOG::Println("Saving {}", ss.str());
  this->Internals->Canvas->SaveAs(ss.str());
}

void Beams::SaveDataSet(int num)
{
  auto mpi = pilot::mpi::Environment::Get();
  std::stringstream ss;
  std::stringstream numSs;
  numSs << std::setw(3) << std::setfill('0') << num;
  ss << this->Internals->Scene->Id << "_" << mpi->Rank << "_" << numSs.str() << ".vtk";
  LOG::Println("Saving {}", ss.str());
  pilot::io::DataSetUtils::Write(this->Internals->Scene->DataSet, ss.str());
}

#undef CHECK_RESULT

} // namespace beams
