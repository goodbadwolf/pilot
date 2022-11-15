cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
include(ExternalProject)

project(Pilot)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(VTKm REQUIRED QUIET)

# Add `fmt` as a in-source thirdparty dependency
ExternalProject_Add(fmt
  SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/fmt"
  CMAKE_CACHE_ARGS
      -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=TRUE
      -DCMAKE_BUILD_TYPE:STRING=Release
      -DFMT_TEST:BOOL=OFF
      -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_CURRENT_BINARY_DIR}/thirdparty/fmt
)
ExternalProject_Get_property(fmt SOURCE_DIR)
set(FMT_INCLUDE "${SOURCE_DIR}/include")
set(FMT_LIB "${CMAKE_CURRENT_BINARY_DIR}/thirdparty/fmt/lib/libfmt.a")

# Add `imgui` as a in-source thirdparty dependency
#ExternalProject_Add(imgui
  #SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/imgui/src"
  #INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/thirdparty/imgui"
  #CMAKE_CACHE_ARGS
  #    -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=TRUE
  #    -DCMAKE_BUILD_TYPE:STRING=Release
  #    -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_CURRENT_BINARY_DIR}/thirdparty/imgui)
  #ExternalProject_Get_property(imgui INSTALL_DIR)
  #set(IMGUI_INCLUDE "${INSTALL_DIR}/include")
  #set(IMGUI_LIB "${CMAKE_CURRENT_BINARY_DIR}/thirdparty/imgui/lib/cmake/imgui_cmake/libimgui.so")

option(PILOT_ENABLE_BEAMS "Enable the Beams app" ON)
option(PILOT_ENABLE_CONVERT "Enable the Convert app" ON)

# CUDA flags
set(CMAKE_CUDA_SEPARABLE_COMPILATION ON)

# Main library
add_subdirectory(pilot)

# Apps
add_subdirectory(beams)
add_subdirectory(convert)