include(FetchContent)

message(STATUS "Fetching VTKm...")

set(VTKm_ENABLE_MPI ON CACHE BOOL "Enable MPI for VTKm")
set(VTKm_ENABLE_OPENMP ON CACHE BOOL "Enable OpenMP backend for VTKm")
set(VTKm_ENABLE_TESTING OFF CACHE BOOL "Disable tests for VTKm")

FetchContent_MakeAvailable(VTKm)

message(STATUS "Fetching VTKm... Done")