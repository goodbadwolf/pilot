#include "Chunk.h"
#include "Transform.h"

#include <pilot/Result.h>
#include <pilot/StringUtils.h>
#include <pilot/io/DataSetUtils.h>
#include <pilot/system/SystemUtils.h>

#include <vtkm/cont/Initialize.h>
#include <vtkm/filter/FieldSelection.h>
#include <vtkm/io/VTKDataSetReader.h>

#include <args.hxx>

#include <array>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

struct Vec3f_32Reader
{
  void operator()(const std::string&, const std::string& value, vtkm::Vec3f_32& vec)
  {
    int start = 0;
    for (int i = 0; i < 3; ++i)
    {
      int end = value.find(",", start);
      if (end == -1)
      {
        end = value.length();
      }
      std::string component = value.substr(start, end);
      vec[i] = std::stof(component);
      start = end + 1;
    }
  }
};

namespace pilot
{
namespace apps
{
namespace convert
{
struct Options
{
  std::string Command;
  std::string InputFileName;
  std::vector<std::string> FieldNames;
  std::string OutputFileNamePrefix;
  ChunkCellSetType OutputCellSetType;
  vtkm::Vec3f_32 Translation;

  int ChunksX;
  int ChunksY;
  int ChunksZ;

  friend std::ostream& operator<<(std::ostream& ostream, const Options& opts);
};

std::ostream& operator<<(std::ostream& ostream, const Options& opts)
{
  std::cerr << "Options {"
            << "InputFileName = " << opts.InputFileName << ", FieldNames = ["
            << pilot::StringUtils::Join(opts.FieldNames, ", ") << "]"
            << ", OutputFileNamePrefix = " << opts.OutputFileNamePrefix
            << ", OutputCellSetType = " << opts.OutputCellSetType << ", ChunksX = " << opts.ChunksX
            << ", ChunksY = " << opts.ChunksY << ", ChunksZ = " << opts.ChunksZ << "}";
  return ostream;
}

Options ParseOptions(int& argc, char** argv)
{
  args::ArgumentParser parser("Converter program.");
  args::Group commandsGrp(parser, "commands");
  args::Command chunkCmd(commandsGrp, "chunk", "chunk the given vtk file");
  args::Command transformCmd(commandsGrp, "transform", "tranform the points in a dataset");
  args::Group argsGrp(
    parser, "arguments", args::Group::Validators::DontCare, args::Options::Global);
  args::ValueFlag<std::string> inputFileNameArg(
    argsGrp, "filename", "The input file", { 'i', "input" }, args::Options::Required);
  args::ValueFlag<int> chunkXArg(argsGrp, "chunks in x", "Number of chunks along X", { "cx" });
  args::ValueFlag<int> chunkYArg(argsGrp, "chunks in y", "Number of chunks along Y", { "cy" });
  args::ValueFlag<int> chunkZArg(argsGrp, "chunks in z", "Number of chunks along Z", { "cz" });
  args::ValueFlagList<std::string> fieldsArgs(argsGrp,
                                              "field to pass",
                                              "A field to pass to the output",
                                              { "f", "field" },
                                              {},
                                              args::Options::Required);
  args::ValueFlag<std::string> outputFileNamePrefixArg(argsGrp,
                                                       "filename pattern",
                                                       "The output file name prefix",
                                                       { 'o', "output" },
                                                       args::Options::Required);
  std::unordered_map<std::string, ChunkCellSetType> cellSetMap{
    { "rectilinear", ChunkCellSetType::Rectilinear }, { "uniform", ChunkCellSetType::Uniform }
  };
  args::MapFlag<std::string, ChunkCellSetType> outputCellSetTypeArg(
    argsGrp, "CellSet type", "The output cellset type", { "outputCellSetType" }, cellSetMap);
  args::ValueFlag<vtkm::Vec3f_32, Vec3f_32Reader> translationArg(
    argsGrp, "translation", "Translation of origin", { "translate" });
  args::HelpFlag helpArg(argsGrp, "help", "Display this help menu", { 'h', "help" });
  try
  {
    parser.ParseCLI(argc, argv);
  }
  catch (const args::Help& _)
  {
    std::cerr << parser << std::endl;
    exit(EXIT_SUCCESS);
  }
  catch (const args::ParseError& e)
  {
    std::cerr << parser << "\n\n";
    std::cerr << e.what() << std::endl;
    exit(EXIT_FAILURE);
  }
  catch (const args::RequiredError& e)
  {
    std::cerr << e.what() << std::endl;
    std::cerr << parser << "\n\n";
    exit(EXIT_FAILURE);
  }

  Options opts;
  if (chunkCmd)
  {
    opts.Command = "chunk";
    opts.InputFileName = args::get(inputFileNameArg);
    opts.FieldNames = args::get(fieldsArgs);
    opts.ChunksX = args::get(chunkXArg);
    opts.ChunksY = args::get(chunkYArg);
    opts.ChunksZ = args::get(chunkZArg);
    opts.OutputFileNamePrefix = args::get(outputFileNamePrefixArg);
    opts.OutputCellSetType = args::get(outputCellSetTypeArg);
  }
  else if (transformCmd)
  {
    opts.Command = "transform";
    opts.InputFileName = args::get(inputFileNameArg);
    opts.FieldNames = args::get(fieldsArgs);
    opts.Translation = args::get(translationArg);
    opts.OutputFileNamePrefix = args::get(outputFileNamePrefixArg);
  }

  return opts;
}
}
}
}

namespace convert = pilot::apps::convert;

int main(int argc, char** argv)
{
  convert::Options opts = convert::ParseOptions(argc, argv);
  auto vtkmOpts = vtkm::cont::InitializeOptions::DefaultAnyDevice;
  vtkm::cont::Initialize(argc, argv, vtkmOpts);

  if (opts.Command == "chunk")
  {
    auto readResult = pilot::io::DataSetUtils::Read(opts.InputFileName);
    if (readResult.IsFailure())
    {
      pilot::system::Die(readResult.Error);
    }

    auto dataSet = readResult.Value;
    for (const auto& fieldName : opts.FieldNames)
    {
      if (!dataSet.HasField(fieldName))
      {
        pilot::system::Die("DataSet does not have field " + fieldName);
      }
    }

    auto fieldSelection = vtkm::filter::FieldSelection();
    for (const auto& fieldName : opts.FieldNames)
    {
      fieldSelection.AddField(fieldName);
    }
    auto chunkResult = convert::ChunkDataSet(
      dataSet, opts.ChunksX, opts.ChunksY, opts.ChunksZ, fieldSelection, opts.OutputCellSetType);
    if (chunkResult.IsFailure())
    {
      pilot::system::Die(chunkResult.Error);
    }
    auto saveResult = convert::SaveChunksToDisk(
      chunkResult.Value, opts.OutputFileNamePrefix, vtkm::io::FileType::ASCII);

    if (saveResult.IsFailure())
    {
      pilot::system::Die(saveResult.Error);
    }
  }
  else if (opts.Command == "transform")
  {
    auto readResult = pilot::io::DataSetUtils::Read(opts.InputFileName);
    if (readResult.IsFailure())
    {
      pilot::system::Die(readResult.Error);
    }

    auto dataSet = readResult.Value;
    for (const auto& fieldName : opts.FieldNames)
    {
      if (!dataSet.HasField(fieldName))
      {
        pilot::system::Die("DataSet does not have field " + fieldName);
      }
    }
    auto fieldSelection = vtkm::filter::FieldSelection();
    for (const auto& fieldName : opts.FieldNames)
    {
      fieldSelection.AddField(fieldName);
    }

    auto transformResult = convert::TransformDataSet(dataSet, opts.Translation, fieldSelection);
    if (!transformResult)
    {
      pilot::system::Die(transformResult.Error);
    }

    std::string fileName = opts.OutputFileNamePrefix + ".vtk";
    auto saveResult =
      pilot::io::DataSetUtils::Write(transformResult.Value, fileName, vtkm::io::FileType::ASCII);
    if (!saveResult)
    {
      pilot::system::Die(saveResult.Error);
    }
  }

  return EXIT_SUCCESS;
}