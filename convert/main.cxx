#include "Chunk.h"
#include <pilot/DataSetUtils.h>
#include <pilot/Result.h>
#include <pilot/StringUtils.h>
#include <pilot/System.h>
#include <pilot/utils/args.hxx>

#include <vtkm/cont/Initialize.h>
#include <vtkm/filter/FieldSelection.h>
#include <vtkm/io/VTKDataSetReader.h>

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace pilot
{
namespace apps
{
namespace convert
{
struct Options
{
  std::string InputFileName;
  std::vector<std::string> FieldNames;
  std::string OutputFileNamePrefix;
  ChunkCellSetType OutputCellSetType;
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
  args::Group argsGrp(
    parser, "arguments", args::Group::Validators::DontCare, args::Options::Global);
  args::ValueFlag<std::string> inputFileNameArg(
    argsGrp, "filename", "The input file", { 'i', "input" }, args::Options::Required);
  args::ValueFlag<int> chunkXArg(
    argsGrp, "chunks in x", "Number of chunks along X", { "cx" }, args::Options::Required);
  args::ValueFlag<int> chunkYArg(
    argsGrp, "chunks in y", "Number of chunks along Y", { "cy" }, args::Options::Required);
  args::ValueFlag<int> chunkZArg(
    argsGrp, "chunks in z", "Number of chunks along Z", { "cz" }, args::Options::Required);
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
  args::MapFlag<std::string, ChunkCellSetType> outputCellSetTypeArg(argsGrp,
                                                                    "CellSet type",
                                                                    "The output cellset type",
                                                                    { "outputCellSetType" },
                                                                    cellSetMap,
                                                                    args::Options::Required);
  args::HelpFlag helpArg(argsGrp, "help", "Display this help menu", { 'h', "help" });
  try
  {
    parser.ParseCLI(argc, argv);
  }
  catch (const args::Help&)
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
    opts.InputFileName = args::get(inputFileNameArg);
    opts.FieldNames = args::get(fieldsArgs);
    opts.ChunksX = args::get(chunkXArg);
    opts.ChunksY = args::get(chunkYArg);
    opts.ChunksZ = args::get(chunkZArg);
    opts.OutputFileNamePrefix = args::get(outputFileNamePrefixArg);
    opts.OutputCellSetType = args::get(outputCellSetTypeArg);
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
  std::cerr << "Running Convert with options: " << opts << "\n";
  auto vtkmOpts = vtkm::cont::InitializeOptions::DefaultAnyDevice;
  vtkm::cont::Initialize(argc, argv, vtkmOpts);

  auto readResult = pilot::DataSetUtils::Read(opts.InputFileName);
  if (readResult.IsError())
  {
    pilot::system::Fail(readResult.Error);
  }

  auto dataSet = readResult.Outcome;
  for (const auto& fieldName : opts.FieldNames)
  {
    if (!dataSet.HasField(fieldName))
    {
      pilot::system::Fail("DataSet does not have field " + fieldName);
    }
  }

  auto fieldSelection = vtkm::filter::FieldSelection();
  for (const auto& fieldName : opts.FieldNames)
  {
    fieldSelection.AddField(fieldName);
  }
  auto chunkResult = convert::ChunkDataSet(
    dataSet, opts.ChunksX, opts.ChunksY, opts.ChunksZ, fieldSelection, opts.OutputCellSetType);
  if (chunkResult.IsError())
  {
    pilot::system::Fail(chunkResult.Error);
  }
  auto saveResult = convert::SaveChunksToDisk(
    chunkResult.Outcome, opts.OutputFileNamePrefix, vtkm::io::FileType::BINARY);

  if (saveResult.IsError())
  {
    pilot::system::Fail(saveResult.Error);
  }

  return EXIT_SUCCESS;
}