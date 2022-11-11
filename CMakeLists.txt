cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

project(Pilot)

#Find the VTK-m package
find_package(VTKm REQUIRED QUIET)

set (PILOT_SRCS
  pilot/test.cxx
)

set (PILOT_HEADERS
  pilot/utils/args.hxx
)

add_library(Pilot ${PILOT_SRCS} ${PILOT_HEADERS})
target_link_libraries(Pilot 
  PRIVATE vtkm_filter 
  PRIVATE vtkm_io
)
# Is this the right way to export the headers?
target_include_directories(Pilot 
  PUBLIC pilot/..)

set (CONVERT_SRCS
  convert/main.cxx
)

add_executable(Convert ${CONVERT_SRCS})
target_link_libraries(Convert 
  PUBLIC Pilot 
  PRIVATE vtkm_filter 
  PRIVATE vtkm_io
)