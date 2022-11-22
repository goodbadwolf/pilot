include(FetchContent)

message(STATUS "Fetching fmt...")

set(CMAKE_POSITION_INDEPENDENT_CODE TRUE CACHE BOOL "Make position independent executables")
set(FMT_TEST OFF CACHE BOOL "Disable tests")

FetchContent_MakeAvailable(fmt)

message(STATUS "Fetching fmt... Done")