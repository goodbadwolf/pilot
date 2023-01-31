project (Convert LANGUAGES CXX)

if (NOT PILOT_ENABLE_CONVERT)
  return()
endif()

find_package(VTKm REQUIRED QUIET)

set (CONVERT_SRCS
  main.cxx
  Chunk.cxx
  Transform.cxx
)

add_executable(Convert ${CONVERT_SRCS})
install(
  TARGETS Convert 
  DESTINATION "${CMAKE_INSTALL_PREFIX}/"
)
target_link_libraries(Convert 
  PRIVATE Pilot 
  PRIVATE vtkm_filter 
  # PRIVATE vtkm_filter_field_transform
  PRIVATE vtkm_io
  PRIVATE args
)