include(FetchContent)

message(STATUS "Fetching args...")

set(ARGS_BUILD_EXAMPLE OFF CACHE BOOL "Disable example(s) for args")
set(ARGS_BUILD_UNITTESTS OFF CACHE BOOL "Disable unit tests for args")

FetchContent_MakeAvailable(args)

message(STATUS "Fetching args... Done")