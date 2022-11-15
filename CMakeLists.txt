cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

project(Pilot)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(VTKm REQUIRED QUIET)

option(PILOT_ENABLE_CONVERT "Enable the Convert app" ON)

add_subdirectory(pilot)
add_subdirectory(convert)