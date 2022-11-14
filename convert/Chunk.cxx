#include "Chunk.h"

#include <pilot/DataSetUtils.h>
#include <pilot/Debug.h>

#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/filter/resampling/Probe.h>

namespace pilot
{
namespace apps
{
namespace convert
{
ChunkResult ChunkDataSet(const vtkm::cont::DataSet& dataSet,
                         int chunksX,
                         int chunksY,
                         int chunksZ,
                         vtkm::filter::FieldSelection fields)
{
  const std::string PREFIX = std::string("*** ") + PILOT_CURRENT_FUNC_NAME + std::string(" ***");

  vtkm::cont::CoordinateSystem coordSystem = dataSet.GetCoordinateSystem();
  vtkm::cont::UnknownArrayHandle coords = coordSystem.GetDataWithExpectedTypes();

  bool isUniformCoordsArray = coords.IsType<UniformPoints>();
  bool isRectilinearArray = coords.IsType<RectilinearPoints>();

  if (!(isUniformCoordsArray || isRectilinearArray))
  {
    return ChunkResult("Unknown coords ArrayHandle type: " + coords.GetArrayTypeName());
  }

  int totalChunks = chunksX * chunksY * chunksZ;
  vtkm::Bounds bounds = coordSystem.GetBounds();
  vtkm::Vec3f origin = { static_cast<vtkm::FloatDefault>(bounds.X.Min),
                         static_cast<vtkm::FloatDefault>(bounds.Y.Min),
                         static_cast<vtkm::FloatDefault>(bounds.Z.Min) };
  vtkm::FloatDefault dOX = bounds.X.Length() / chunksX;
  vtkm::FloatDefault dOY = bounds.Y.Length() / chunksY;
  vtkm::FloatDefault dOZ = bounds.Z.Length() / chunksZ;
  vtkm::Vec3f chunkSize = { dOX, dOY, dOZ };
  vtkm::Id3 dims;
  vtkm::Vec3f spacing;
  if (isUniformCoordsArray)
  {
    auto coordsAH = coords.AsArrayHandle<UniformPoints>();
    dims = coordsAH.GetDimensions();
    spacing = coordsAH.GetSpacing();
  }
  else if (isRectilinearArray)
  {
    auto coordsAH = coords.AsArrayHandle<RectilinearPoints>();
    dims[0] = coordsAH.GetFirstArray().GetNumberOfValues();
    dims[1] = coordsAH.GetSecondArray().GetNumberOfValues();
    dims[2] = coordsAH.GetThirdArray().GetNumberOfValues();
  }

  vtkm::Id3 chunkDims = dims / vtkm::Id3{ chunksX, chunksY, chunksZ };
  vtkm::Id3 chunkPDims = chunkDims + vtkm::Id3{ 1, 1, 1 };
  vtkm::Vec3f chunkSpacing = chunkSize / chunkDims;

  std::cerr << PREFIX << "\n";
  std::cerr << "Original dataset summary:\n";
  std::cerr << "Bounds     = " << bounds << "\n";
  std::cerr << "Dimensions = " << dims << "\n";
  if (isUniformCoordsArray)
    std::cerr << "Spacing    = " << spacing << "\n";
  std::cerr << "Chunked dataset summary:\n";
  std::cerr << "Chunk Size    = " << chunkSize << "\n";
  std::cerr << "Chunk Dims    = " << chunkDims << "\n";
  std::cerr << "Chunk Spacing = " << chunkSpacing << "\n";

  DataSets dataSets;
  auto index = 0;
  for (auto z = 0; z < chunksZ; ++z)
  {
    for (auto y = 0; y < chunksY; ++y)
    {
      for (auto x = 0; x < chunksX; ++x)
      {
        try
        {
          vtkm::Vec3f chunkOrigin = origin + vtkm::Vec3f{ x * dOX, y * dOY, z * dOZ };
          auto geometry =
            vtkm::cont::DataSetBuilderUniform::Create(chunkPDims, chunkOrigin, chunkSpacing);
          vtkm::filter::resampling::Probe probe;
          probe.SetGeometry(geometry);
          probe.SetFieldsToPass(fields);
          vtkm::cont::DataSet chunk = probe.Execute(dataSet);
          std::cerr << "Chunk " << (index++) << " created at origin = " << chunkOrigin << "\n";
          auto filterResult = FilterFields(chunk, fields);
          chunk = filterResult.Outcome;
          dataSets.push_back(chunk);
        }
        catch (const vtkm::cont::Error& e)
        {
          return ChunkResult(e.GetMessage());
        }
      }
    }
  }
  std::cerr << std::endl;

  return ChunkResult(dataSets);
}

SaveResult SaveChunksToDisk(const DataSets& dataSets,
                            const std::string& fileNamePrefix,
                            vtkm::io::FileType fileType)
{
  const std::string PREFIX = std::string("*** ") + PILOT_CURRENT_FUNC_NAME + std::string(" ***");

  std::cerr << PREFIX << "\n";
  int index = 0;
  for (auto& dataSet : dataSets)
  {
    std::stringstream ss;
    ss << fileNamePrefix << "." << (index++) << ".vtk";
    std::string fileName = ss.str();
    std::cerr << "Saving dataset at " << fileName << "\n";
    auto result = pilot::DataSetUtils::Write(dataSet, fileName, fileType);
    if (result.IsError())
    {
      return SaveResult(result.Error);
    }
  }

  std::cerr << std::endl;
  return SaveResult(true);
}

FilterResult FilterFields(vtkm::cont::DataSet& dataSet, vtkm::filter::FieldSelection fieldSelection)
{
  vtkm::cont::DataSet filteredDataSet;

  // Copy over coordinate systems from original
  for (const auto& coordSystem : dataSet.GetCoordinateSystems())
  {
    filteredDataSet.AddCoordinateSystem(coordSystem);
  }

  // Copy over cellset from original
  filteredDataSet.SetCellSet(dataSet.GetCellSet());

  // Copy over selected fields
  for (auto i = 0; i < dataSet.GetNumberOfFields(); ++i)
  {
    const auto& field = dataSet.GetField(i);
    if (fieldSelection.HasField(field))
    {
      filteredDataSet.AddField(field);
    }
  }

  return FilterResult(filteredDataSet);
}
}
}
}
