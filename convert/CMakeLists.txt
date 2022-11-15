find_package(VTKm REQUIRED QUIET)

set (CONVERT_SRCS
  main.cxx
  Chunk.cxx
)

if (NOT PILOT_ENABLE_CONVERT)
  return()
endif()

add_executable(Convert ${CONVERT_SRCS})
install(
  TARGETS Convert 
  DESTINATION "${CMAKE_INSTALL_PREFIX}/"
)
target_link_libraries(Convert 
  PUBLIC Pilot 
  PRIVATE vtkm_filter 
  PRIVATE vtkm_io
)