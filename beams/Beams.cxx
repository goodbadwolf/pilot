#include "Beams.h"
#include "Config.h"
#include "SceneFactory.h"
// #include "Window.h"
#include "Profiler.h"
#include "mpi/MpiEnv.h"
#include "rendering/BoundsMap.h"
#include "rendering/Scene.h"
#include "utils/File.h"
#include "utils/Fmt.h"
#include "utils/Json.h"

#include <vtkm/cont/Initialize.h>
#include <vtkm/io/FileUtils.h>
#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/thirdparty/diy/diy.h>
#include <vtkm/thirdparty/diy/mpi-cast.h>

#include "fmt/core.h"

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
struct Beams::InternalsType
{
  InternalsType() = default;

  Result ParseArgs(int argc, char** argv)
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
      if (argKey == "--data-dir")
      {
        this->DataDir = argVal;
      }
      else if (argKey == "--preset")
      {
        this->StartPresetName = argVal;
      }
    }

    if (this->DataDir.length() > 0)
    {
      return Result{ .Success = true };
    }
    else
    {
      return Result{ .Success = false, .Err = "--data-dir not specified" };
    }
  }

  Result LoadConfig()
  {
    const char* CONFIG_FILE_NAME = "config.json";
    std::string configFilePath = vtkm::io::MergePaths(this->DataDir, CONFIG_FILE_NAME);
    if (!beams::io::File::FileExists(configFilePath))
    {
      return Result::Failed(fmt::format("Config file '{}' not found", configFilePath));
    }

    auto result = this->Cfg.LoadFromFile(configFilePath);
    if (!result.Success)
    {
      return Result::Failed(result.Err);
    }

    return Result::Succeeded();
  }

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

  std::string DataDir;
  std::string StartPresetName;
  // beams::Window Win;
  beams::Config Cfg;
  std::string CurrentPresetName;
  std::shared_ptr<beams::rendering::Scene> Scene;
  std::shared_ptr<vtkm::rendering::CanvasRayTracer> Canvas;
  std::shared_ptr<beams::mpi::MpiEnv> Mpi;
  std::shared_ptr<beams::Profiler> Profiler;
};

int ToMs(vtkm::Float64 t)
{
  return static_cast<int>(t * 1000);
}

void PrintDeviceInfo()
{
#ifdef VTKM_ENABLE_OPENMP
  auto mpi = beams::mpi::MpiEnv::Get();
  auto& runtimeConfig = vtkm::cont::RuntimeDeviceInformation{}.GetRuntimeConfiguration(
    vtkm::cont::DeviceAdapterTagOpenMP());
  vtkm::Id maxThreads = 0;
  vtkm::Id numThreads = 0;
  runtimeConfig.GetMaxThreads(maxThreads);
  runtimeConfig.GetThreads(numThreads);
  std::cerr << mpi->Hostname << " => "
            << "MaxThreads = " << maxThreads << ", NumThreads = " << numThreads << "\n";
#endif
}

void PrintTimeStats(const std::string& label, vtkm::Float64 time)
{
  auto mpi = beams::mpi::MpiEnv::Get();
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
    Fmt::Println("{}: Min = {} ms, Max = {} ms, Avg = {} ms", label, minTime, maxTime, avgTime);
  }
}

void PrintRootTimeStat(const std::string& label, vtkm::Float64 time)
{
  auto mpi = beams::mpi::MpiEnv::Get();
  if (mpi->Rank == 0)
  {
    Fmt::Println("{}: Time = {} ms", label, ToMs(time));
  }
}

Beams::Beams(std::shared_ptr<beams::mpi::MpiEnv> mpiEnv)
  : Internals(new InternalsType())
{
  this->Internals->Mpi = mpiEnv;
}

Result Beams::Initialize(int& argc, char** argv)
{
  auto opts = vtkm::cont::InitializeOptions::RequireDevice;
  vtkm::cont::Initialize(argc, argv, opts);
  PrintDeviceInfo();

  CHECK_RESULT(this->Internals->ParseArgs(argc, argv), "Error initializing beams")
  CHECK_RESULT(this->Internals->LoadConfig(), "Error initializing beams");
  /*
  result = this->Internals->Win.Initialize("Beams", 1920, 1080);
  CHECK_RESULT(result)
  */
#undef CHECK_RESULT

  return Result::Succeeded();
}

void Beams::LoadScene()
{
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
  auto mpi = beams::mpi::MpiEnv::Get();
  Fmt::Println0("Scene loaded with global bounds: {}", scene->BoundsMap->GlobalBounds);
}

void Beams::SetupScene()
{
  this->Internals->Canvas->Clear();
  this->Internals->Scene->Ready();
}

void Beams::Run()
{
  auto mpi = beams::mpi::MpiEnv::Get();

  this->LoadScene();
  std::shared_ptr<beams::rendering::Scene> scene = this->Internals->Scene;
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
    Fmt::Println(ss.str());
  }
  PrintTimeStats("Phase 1", Phase1Time);
  PrintTimeStats("Phase 2", Phase2Time);
  PrintTimeStats("Phase 3", Phase3Time);
  PrintTimeStats("Phase 4", Phase4Time);
  PrintRootTimeStat("Phase 5", Phase5Time);

  if (mpi->Rank == 0)
  {
    this->SaveCanvas(0);
    this->SaveDataSet(0);
  }
}

void Beams::SaveCanvas(int num)
{
  auto mpi = beams::mpi::MpiEnv::Get();
  std::stringstream ss;
  std::stringstream numSs;
  numSs << std::setw(3) << std::setfill('0') << num;
  ss << this->Internals->Scene->Id << "_" << mpi->Rank << "_" << numSs.str() << ".png";
  Fmt::Println("Saving {}", ss.str());
  this->Internals->Canvas->SaveAs(ss.str());
}

void Beams::SaveDataSet(int num)
{
  auto mpi = beams::mpi::MpiEnv::Get();
  std::stringstream ss;
  std::stringstream numSs;
  numSs << std::setw(3) << std::setfill('0') << num;
  ss << this->Internals->Scene->Id << "_" << mpi->Rank << "_" << numSs.str() << ".vtk";
  Fmt::Println("Saving {}", ss.str());
  beams::io::File::SaveDataSet(this->Internals->Scene->DataSet, ss.str());
}

} // namespace beams
