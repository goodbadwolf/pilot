#pragma once

#include <pilot/mpi/Environment.h>
#include <pilot/mpi/TopologyShape.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/Timer.h>

#include "fmt/core.h"

#include <string>

#define LOG_VAR(x) beams::Logger::Println(#x " = {}", x);

#define LOG_VAR0(x) beams::Logger::Println0(#x " = {}", x);

#define LOG_VEC(v)                   \
  beams::Logger::PrintArrayHandleln( \
    #v, vtkm::cont::make_ArrayHandle(v, vtkm::CopyFlag::Off), false);

#define LOG_VEC0(v)                   \
  beams::Logger::PrintArrayHandleln0( \
    #v, vtkm::cont::make_ArrayHandle(v, vtkm::CopyFlag::Off), false);

#define LOG_VEC_F(v) \
  beams::Logger::PrintArrayHandleln(#v, vtkm::cont::make_ArrayHandle(v, vtkm::CopyFlag::Off), true);

#define LOG_VEC0_F(v)                 \
  beams::Logger::PrintArrayHandleln0( \
    #v, vtkm::cont::make_ArrayHandle(v, vtkm::CopyFlag::Off), true);

#define LOG_ARR(a) beams::Logger::PrintArrayHandleln(#a, a);

#define LOG_ARR0(a) beams::Logger::PrintArrayHandleln0(#a, a);

#define LOG_ARR_F(a) beams::Logger::PrintArrayHandleln(#a, a, true);

#define LOG_ARR0_F(a) beams::Logger::PrintArrayHandleln0(#a, a, true);

#define LOG_TMR(x) beams::Logger::PrintTimerln(#x, x);

#define LOG_TMR0(x) beams::Logger::PrintTimerln0(#x, x);

namespace beams
{
struct Logger
{
  template <typename S, typename... Args>
  static void Print(const S& format_str, Args&&... args)
  {
    std::string s = fmt::format(format_str, args...);
    auto mpi = pilot::mpi::Environment::Get();
    if (mpi->Shape == pilot::mpi::TopologyShape::Line)
    {
      fmt::print(stderr, "{} ({}): {}", mpi->Rank, mpi->XRank, s);
    }
    else if (mpi->Shape == pilot::mpi::TopologyShape::Rectangle)
    {
      fmt::print(stderr, "{} ({}, {}): {}", mpi->Rank, mpi->XRank, mpi->YRank, s);
    }
    else if (mpi->Shape == pilot::mpi::TopologyShape::Cuboid)
    {
      fmt::print(stderr, "{} ({}, {}, {}): {}", mpi->Rank, mpi->XRank, mpi->YRank, mpi->ZRank, s);
    }
    else
    {
      fmt::print(stderr, "(MPI Topology not set): {}", s);
    }
  }

  template <typename S, typename... Args>
  static void Println(const S& format_str, Args&&... args)
  {
    Logger::Print(format_str + std::string("\n"), args...);
  }

  template <typename S, typename... Args>
  static void Println0(const S& format_str, Args&&... args)
  {
    auto mpi = pilot::mpi::Environment::Get();
    if (mpi->Rank == 0)
    {
      Logger::Println(format_str, args...);
    }
  }

  template <typename S, typename... Args>
  static void Printlnr(int rank, const S& format_str, Args&&... args)
  {
    auto mpi = pilot::mpi::Environment::Get();
    if (mpi->Rank == rank)
    {
      Logger::Println(format_str, args...);
    }
  }

  template <typename S>
  static void PrintTimerln(const S& name, const vtkm::cont::Timer& timer)
  {
    Logger::Println(name + std::string(": {} ms"), Logger::ToMs(timer.GetElapsedTime()));
  }

  template <typename S>
  static void PrintTimerln0(const S& name, const vtkm::cont::Timer& timer)
  {
    Logger::Println0(name + std::string(": {} ms"), Logger::ToMs(timer.GetElapsedTime()));
  }

  template <typename S, typename ArrayHandleType>
  static void PrintArrayHandleln(const S& name, const ArrayHandleType& array, bool full = false)
  {
    std::stringstream ss;
    ss << name << ": ";
    vtkm::cont::printSummary_ArrayHandle(array, ss, full);
    Logger::Print(ss.str());
  }

  template <typename S, typename ArrayHandleType>
  static void PrintArrayHandleln0(const S& name, const ArrayHandleType& array, bool full = false)
  {
    auto mpi = pilot::mpi::Environment::Get();
    if (mpi->Rank == 0)
    {
      Logger::PrintArrayHandleln(name, array, full);
    }
  }

  template <typename S, typename ArrayHandleType>
  static void PrintArrayHandlelnr(int rank,
                                  const S& name,
                                  const ArrayHandleType& array,
                                  bool full = false)
  {
    auto mpi = pilot::mpi::Environment::Get();
    if (mpi->Rank == rank)
    {
      Logger::PrintArrayHandleln(name, array, full);
    }
  }

  template <typename Precision>
  static int ToMs(Precision timeInSeconds)
  {
    return static_cast<int>(timeInSeconds * 1000);
  }
};
}

using LOG = beams::Logger;