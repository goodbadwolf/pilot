#include "Chunk.h"
#include "IO.h"
#include "Result.h"
#include "String.h"
#include <pilot/utils/args.hxx>

#include <vtkm/cont/Initialize.h>
#include <vtkm/filter/FieldSelection.h>
#include <vtkm/io/VTKDataSetReader.h>

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

struct Options
{
  std::string InputFileName;
  std::vector<std::string> FieldNames;
  std::string OutputFileNamePrefix;
  int ChunksX;
  int ChunksY;
  int ChunksZ;

  friend std::ostream& operator<<(std::ostream& ostream, const Options& opts);
};

std::ostream& operator<<(std::ostream& ostream, const Options& opts)
{
  std::cerr << "Options {"
            << "InputFileName = " << opts.InputFileName << ", FieldNames = ["
            << Join(opts.FieldNames, ", ") << "]"
            << ", OutputFileNamePrefix = " << opts.OutputFileNamePrefix
            << ", ChunksX = " << opts.ChunksX << ", ChunksY = " << opts.ChunksY
            << ", ChunksZ = " << opts.ChunksZ << "}";
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
    opts.OutputFileNamePrefix = args::get(outputFileNamePrefixArg);
    opts.ChunksX = args::get(chunkXArg);
    opts.ChunksY = args::get(chunkYArg);
    opts.ChunksZ = args::get(chunkZArg);
  }

  return opts;
}

int main(int argc, char** argv)
{
  Options opts = ParseOptions(argc, argv);
  auto vtkmOpts = vtkm::cont::InitializeOptions::DefaultAnyDevice;
  vtkm::cont::Initialize(argc, argv, vtkmOpts);

  auto readResult = ReadDataSet(opts.InputFileName);
  if (readResult.IsError())
  {
    Fail(readResult.Error);
  }

  auto dataSet = readResult.Outcome;
  auto fieldSelection = vtkm::filter::FieldSelection();
  for (const auto& fieldName : opts.FieldNames)
  {
    fieldSelection.AddField(fieldName);
  }
  auto chunkResult =
    ChunkDataSet(dataSet, opts.ChunksX, opts.ChunksY, opts.ChunksZ, fieldSelection);
  if (chunkResult.IsError())
  {
    Fail(chunkResult.Error);
  }
  auto saveResult =
    SaveChunksToDisk(chunkResult.Outcome, opts.OutputFileNamePrefix, vtkm::io::FileType::BINARY);
  return 0;
}