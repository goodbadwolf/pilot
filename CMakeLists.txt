cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

project(Pilot)

#Find the VTK-m package
find_package(VTKm REQUIRED QUIET)

set (SOURCES
  pilot/test.cxx
)

add_library(Pilot ${SOURCES})
target_link_libraries(Pilot vtkm_filter vtkm_io)