include(FetchContent)

# args
FetchContent_Declare(
    args 
    GIT_REPOSITORY https://github.com/Taywee/args
    GIT_TAG        6.3.0
)
add_subdirectory(args)

# fmt
FetchContent_Declare(
  fmt
  GIT_REPOSITORY        https://github.com/fmtlib/fmt
  GIT_TAG               9.1.0
  OVERRIDE_FIND_PACKAGE
)
add_subdirectory(fmt)

# VTKm 
FetchContent_Declare(
  VTKm 
  GIT_REPOSITORY        https://gitlab.kitware.com/vtk/vtk-m 
  GIT_TAG               v1.9.0
  OVERRIDE_FIND_PACKAGE
)
add_subdirectory(vtkm)

# toml++
FetchContent_Declare(
    tomlplusplus
    GIT_REPOSITORY        https://github.com/marzer/tomlplusplus
    GIT_TAG               v3.2.0
    OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(tomlplusplus)
add_subdirectory(tomlplusplus)