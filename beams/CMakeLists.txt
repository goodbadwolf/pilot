project(Beams CXX)

find_package(VTKm REQUIRED QUIET)
# vtkm_find_gl(OPTIONAL GL GLEW)
# find_package(glfw3 3.3 REQUIRED)

if (NOT PILOT_ENABLE_BEAMS)
  return()
endif()

if(TARGET vtkm_rendering AND
  # TARGET OpenGL::GL AND
  # TARGET GLEW::GLEW AND
   VTKm_ENABLE_MPI)

  set (BEAMS_SOURCES
  Beams.cxx
  Config.cxx
  Intersections.cxx
  main.cxx
  Profiler.cxx
  SceneFactory.cxx
  Timer.cxx
  # Window.cxx

  mpi/MpiEnv.cxx

  rendering/BoundsMap.cxx
  rendering/Light.cxx
  rendering/LightCollection.cxx
  rendering/LightedVolumeRenderer.cxx
  rendering/MapperLightedVolume.cxx
  rendering/Scene.cxx
  rendering/SpheresScene.cxx
  rendering/SubdividedSpheresScene.cxx
  rendering/VortexPatchScene.cxx
  rendering/FileSceneBase.cxx
  rendering/EngineScene.cxx
  rendering/BonsaiScene.cxx

  sources/Spheres.cxx
  utils/File.cxx
  )

  set (BEAMS_INCLUDE .)

  set(CMAKE_CUDA_SEPARABLE_COMPILATION ON)

  add_executable(Beams ${BEAMS_SOURCES})
  add_dependencies(Beams fmt)
  # add_dependencies(Beams imgui)
  target_include_directories(Beams PUBLIC ${BEAMS_INCLUDE})
  target_include_directories(Beams PUBLIC ${FMT_INCLUDE})
  # target_include_directories(Beams PUBLIC ${IMGUI_INCLUDE})
  target_link_libraries(Beams PRIVATE vtkm_rendering vtkm_source)
  # target_link_libraries(Beams PRIVATE glfw GLEW::GLEW OpenGL::GL)
  # target_link_libraries(Beams PRIVATE ${IMGUI_LIB})
  target_link_libraries(Beams PRIVATE ${FMT_LIB})
  target_link_libraries(Beams PRIVATE MPI::MPI_CXX)
  vtkm_add_target_information(Beams
                              DROP_UNUSED_SYMBOLS
                              MODIFY_CUDA_FLAGS
                              DEVICE_SOURCES ${BEAMS_SOURCES})
endif()