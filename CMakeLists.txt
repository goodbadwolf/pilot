cmake_minimum_required(VERSION 3.24 FATAL_ERROR)

project(Pilot)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(PILOT_ENABLE_SCCACHE "Use sccache to speed up compilation process" ON)

if(PILOT_ENABLE_SCCACHE)
  find_program(SCCACHE_PATH sccache)
  if(SCCACHE_PATH)
    message(STATUS "Using sccache at ${SCCACHE_PATH}")
    set(CMAKE_C_COMPILER_LAUNCHER ${SCCACHE_PATH})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${SCCACHE_PATH})
  else()
    message(STATUS "Could not find sccache")
  endif()
endif()

# CUDA flags
set(CMAKE_CUDA_SEPARABLE_COMPILATION ON)

# Dependencies
add_subdirectory(thirdparty)

# Main library
add_subdirectory(pilot)

# Apps
option(PILOT_ENABLE_BEAMS "Enable the Beams app" ON)
option(PILOT_ENABLE_CONVERT "Enable the Convert app" ON)

add_subdirectory(beams)
add_subdirectory(convert)