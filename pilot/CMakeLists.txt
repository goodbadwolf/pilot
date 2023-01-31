find_package(VTKm REQUIRED QUIET)
find_package(tomlplusplus REQUIRED QUIET)
find_package(fmt REQUIRED QUIET)

set (PILOT_SRCS
  io/DataSetUtils.cxx
  io/FileSystemUtils.cxx

  mpi/Environment.cxx

  staging/NamedDescriptor.cxx
  staging/Stage.cxx
  staging/TOMLDescriptor.cxx
  staging/dataset_providers/Descriptor.cxx
  staging/dataset_providers/Factory.cxx
  staging/dataset_providers/SubdividedSpheresDescriptor.cxx

  system/SystemUtils.cxx
)

set (PILOT_HEADERS
  io/DataSetUtils.h
  io/FileSystemUtils.h

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

target_link_libraries(Pilot PRIVATE tomlplusplus::tomlplusplus)

target_link_libraries(Pilot PRIVATE fmt::fmt)

target_link_libraries(Pilot 
  PRIVATE vtkm_filter 
  PRIVATE vtkm_io
)

# Is this the right way to export the headers?
target_include_directories(Pilot 
  PUBLIC ..)