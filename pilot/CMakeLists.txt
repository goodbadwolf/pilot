# find_package(VTKm REQUIRED QUIET)

set (PILOT_SRCS
  io/DataSetUtils.cxx
  io/PathUtils.cxx

  mpi/Environment.cxx
)

set (PILOT_HEADERS
  io/DataSetUtils.h
  io/PathUtils.h

  mpi/Environment.h
  mpi/TopologyShape.h

  system/SystemUtils.h

  Debug.h
  Result.h
  StringUtils.h
)

add_library(Pilot ${PILOT_SRCS} ${PILOT_HEADERS})
install(
  TARGETS Pilot 
  DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/Pilot
)
install(
  DIRECTORY "${CMAKE_SOURCE_DIR}/pilot"
  DESTINATION "${CMAKE_INSTALL_PREFIX}/include"
  FILES_MATCHING
  PATTERN "*.h"
)
target_link_libraries(Pilot 
  PRIVATE vtkm_filter 
  PRIVATE vtkm_io
)
# Is this the right way to export the headers?
target_include_directories(Pilot 
  PUBLIC ..)