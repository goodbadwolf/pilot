#include "Chunk.h"

#include <pilot/Debug.h>
#include <pilot/io/DataSetUtils.h>

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/DataSetBuilderRectilinear.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/filter/resampling/Probe.h>

namespace pilot
{
namespace apps
{
namespace convert
{
using UniformPoints = vtkm::cont::ArrayHandleUniformPointCoordinates;
using FloatHandle = vtkm::cont::ArrayHandle<vtkm::FloatDefault>;
using RectilinearPoints =
  vtkm::cont::ArrayHandleCartesianProduct<FloatHandle, FloatHandle, FloatHandle>;

FloatHandle CreateCoordArray(vtkm::Id size, vtkm::FloatDefault start, vtkm::FloatDefault spacing)
{
  auto coordArrayTmp = vtkm::cont::ArrayHandleCounting<vtkm::FloatDefault>(start, spacing, size);
  FloatHandle coordArray;
  vtkm::cont::Algorithm::Copy(coordArrayTmp, coordArray);
  return coordArray;
}

vtkm::cont::DataSet CreateUniformGeometry(vtkm::Id3 chunkPDims,
                                          vtkm::Vec3f chunkOrigin,
                                          vtkm::Vec3f chunkSpacing)
{
  return vtkm::cont::DataSetBuilderUniform::Create(chunkPDims, chunkOrigin, chunkSpacing);
}

vtkm::cont::DataSet CreateRectilinearGeometry(vtkm::Id3 chunkPDims,
                                              vtkm::Vec3f chunkOrigin,
                                              vtkm::Vec3f chunkSpacing)
{
  FloatHandle xCoords, yCoords, zCoords;
  xCoords = CreateCoordArray(chunkPDims[0], chunkOrigin[0], chunkSpacing[0]);
  yCoords = CreateCoordArray(chunkPDims[1], chunkOrigin[1], chunkSpacing[1]);
  zCoords = CreateCoordArray(chunkPDims[2], chunkOrigin[2], chunkSpacing[2]);
  return vtkm::cont::DataSetBuilderRectilinear::Create(xCoords, yCoords, zCoords);
}

ChunkResult ChunkDataSet(const vtkm::cont::DataSet& dataSet,
                         int chunksX,
                         int chunksY,
                         int chunksZ,
                         vtkm::filter::FieldSelection fields,
                         ChunkCellSetType chunkCellSetType)
{
  const std::string PREFIX = std::string("*** ") + PILOT_CURRENT_FUNC_NAME + std::string(" ***");

  vtkm::cont::CoordinateSystem coordSystem = dataSet.GetCoordinateSystem();
  vtkm::cont::UnknownArrayHandle coords = coordSystem.GetDataWithExpectedTypes();

  bool isUniformCoordsArray = coords.IsType<UniformPoints>();
  bool isRectilinearArray = coords.IsType<RectilinearPoints>();

  if (!(isUniformCoordsArray || isRectilinearArray))
  {
    return ChunkResult::Fail("Unknown coords ArrayHandle type: " + coords.GetArrayTypeName());
  }

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
  std::cerr << "Number of chunks = " << (chunksX * chunksY * chunksZ) << "\n";

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
          vtkm::cont::DataSet geometry;
          if (chunkCellSetType == ChunkCellSetType::Rectilinear)
          {
            geometry = CreateRectilinearGeometry(chunkPDims, chunkOrigin, chunkSpacing);
          }
          else if (chunkCellSetType == ChunkCellSetType::Uniform)
          {
            geometry = CreateUniformGeometry(chunkPDims, chunkOrigin, chunkSpacing);
          }
          else
          {
            return ChunkResult::Fail("Unsupported chunk CellSet type");
          }

          vtkm::filter::resampling::Probe probe;
          probe.SetGeometry(geometry);
          probe.SetFieldsToPass(fields);
          vtkm::cont::DataSet chunk = probe.Execute(dataSet);
          std::cerr << "Chunk " << (index++) << " created at origin = " << chunkOrigin << "\n";
          auto filterResult = FilterFields(chunk, fields);
          chunk = filterResult.Value;
          dataSets.push_back(chunk);
        }
        catch (const vtkm::cont::Error& e)
        {
          return ChunkResult::Fail(e.GetMessage());
        }
      }
    }
  }
  std::cerr << std::endl;

  return ChunkResult::Success(dataSets);
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
    auto result = pilot::io::DataSetUtils::Write(dataSet, fileName, fileType);
    if (result.IsFailure())
    {
      return SaveResult::Fail(result.Error);
    }
  }

  std::cerr << std::endl;
  return SaveResult::Success(true);
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

  return FilterResult::Success(filteredDataSet);
}

std::ostream& operator<<(std::ostream& os, const ChunkCellSetType& cellSetType)
{
  std::string typeStr;
  switch (cellSetType)
  {
    case ChunkCellSetType::Rectilinear:
      typeStr = "rectilinear";
      break;

    case ChunkCellSetType::Uniform:
      typeStr = "uniform";
      break;
  }
  os << typeStr;
  return os;
}
}
}
}
