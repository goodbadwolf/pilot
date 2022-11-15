#ifndef beams_fmt_utils_h
#define beams_fmt_utils_h

#include "../mpi/MpiEnv.h"

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/Timer.h>

#include "fmt/core.h"

#include <functional>
#include <string>

#define FMT_VAR(x) Fmt::Println(#x " = {}", x);

#define FMT_VAR0(x) Fmt::Println0(#x " = {}", x);

#define FMT_VEC(v) \
  Fmt::PrintArrayHandleln(#v, vtkm::cont::make_ArrayHandle(v, vtkm::CopyFlag::Off), false);

#define FMT_VEC0(v) \
  Fmt::PrintArrayHandleln0(#v, vtkm::cont::make_ArrayHandle(v, vtkm::CopyFlag::Off), false);

#define FMT_VEC_F(v) \
  Fmt::PrintArrayHandleln(#v, vtkm::cont::make_ArrayHandle(v, vtkm::CopyFlag::Off), true);

#define FMT_VEC0_F(v) \
  Fmt::PrintArrayHandleln0(#v, vtkm::cont::make_ArrayHandle(v, vtkm::CopyFlag::Off), true);

#define FMT_ARR(a) Fmt::PrintArrayHandleln(#a, a);

#define FMT_ARR0(a) Fmt::PrintArrayHandleln0(#a, a);

#define FMT_ARR_F(a) Fmt::PrintArrayHandleln(#a, a, true);

#define FMT_ARR0_F(a) Fmt::PrintArrayHandleln0(#a, a, true);

#define FMT_TMR(x) Fmt::PrintTimerln(#x, x);

#define FMT_TMR0(x) Fmt::PrintTimerln0(#x, x);

struct Fmt
{
  template <typename S, typename... Args>
  static void Print(const S& format_str, Args&&... args)
  {
    std::string s = fmt::format(format_str, args...);
    auto mpi = beams::mpi::MpiEnv::Get();
    if (mpi->Shape == beams::mpi::MpiEnv::TopologyShape::Line)
    {
      fmt::print(stderr, "{} ({}): {}", mpi->Rank, mpi->XRank, s);
    }
    else if (mpi->Shape == beams::mpi::MpiEnv::TopologyShape::Rectangle)
    {
      fmt::print(stderr, "{} ({}, {}): {}", mpi->Rank, mpi->XRank, mpi->YRank, s);
    }
    else if (mpi->Shape == beams::mpi::MpiEnv::TopologyShape::Cuboid)
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
    Fmt::Print(format_str + std::string("\n"), args...);
  }

  template <typename S, typename... Args>
  static void Println0(const S& format_str, Args&&... args)
  {
    auto mpi = beams::mpi::MpiEnv::Get();
    if (mpi->Rank == 0)
    {
      Fmt::Println(format_str, args...);
    }
  }

  template <typename S, typename... Args>
  static void Printlnr(int rank, const S& format_str, Args&&... args)
  {
    auto mpi = beams::mpi::MpiEnv::Get();
    if (mpi->Rank == rank)
    {
      Fmt::Println(format_str, args...);
    }
  }

  template <typename S>
  static void PrintTimerln(const S& name, const vtkm::cont::Timer& timer)
  {
    Fmt::Println(name + std::string(": {} ms"), Fmt::ToMs(timer.GetElapsedTime()));
  }

  template <typename S>
  static void PrintTimerln0(const S& name, const vtkm::cont::Timer& timer)
  {
    Fmt::Println0(name + std::string(": {} ms"), Fmt::ToMs(timer.GetElapsedTime()));
  }

  template <typename S, typename ArrayHandleType>
  static void PrintArrayHandleln(const S& name, const ArrayHandleType& array, bool full = false)
  {
    std::stringstream ss;
    ss << name << ": ";
    vtkm::cont::printSummary_ArrayHandle(array, ss, full);
    Fmt::Print(ss.str());
  }

  template <typename S, typename ArrayHandleType>
  static void PrintArrayHandleln0(const S& name, const ArrayHandleType& array, bool full = false)
  {
    auto mpi = beams::mpi::MpiEnv::Get();
    if (mpi->Rank == 0)
    {
      Fmt::PrintArrayHandleln(name, array, full);
    }
  }

  template <typename S, typename ArrayHandleType>
  static void PrintArrayHandlelnr(int rank,
                                  const S& name,
                                  const ArrayHandleType& array,
                                  bool full = false)
  {
    auto mpi = beams::mpi::MpiEnv::Get();
    if (mpi->Rank == rank)
    {
      Fmt::PrintArrayHandleln(name, array, full);
    }
  }



  template <typename Precision>
  static int ToMs(Precision timeInSeconds)
  {
    return static_cast<int>(timeInSeconds * 1000);
  }
};

#endif // beams_fmt_utils_h