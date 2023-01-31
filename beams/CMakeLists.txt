project(Beams CXX)

if (NOT PILOT_ENABLE_BEAMS)
  return()
endif()

find_package(VTKm REQUIRED QUIET)
find_package(MPI REQUIRED QUIET)
find_package(fmt REQUIRED QUIET)
find_package(tomlplusplus REQUIRED QUIET)
# vtkm_find_gl(OPTIONAL GL GLEW)
# find_package(glfw3 3.3 REQUIRED)

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

  rendering/BoundsMap.cxx
  rendering/Light.cxx
  rendering/LightCollection.cxx
  rendering/LightedVolumeRenderer.cxx
  rendering/MapperLightedVolume.cxx
  rendering/Scene.cxx
  rendering/SpheresScene.cxx
  #rendering/SubdividedSpheresScene.cxx
  #rendering/VortexPatchScene.cxx
  #rendering/FileSceneBase.cxx
  #rendering/EngineScene.cxx
  #rendering/BonsaiScene.cxx
  #rendering/FootScene.cxx
  #rendering/HeadScene.cxx

  sources/Spheres.cxx
  )

  set (BEAMS_INCLUDE .)

  set(CMAKE_CUDA_SEPARABLE_COMPILATION ON)

  add_executable(Beams ${BEAMS_SOURCES})
  # add_dependencies(Beams fmt)
  # add_dependencies(Beams imgui)
  target_include_directories(Beams PUBLIC ${BEAMS_INCLUDE})
  # target_include_directories(Beams PUBLIC ${FMT_INCLUDE})
  # target_include_directories(Beams PUBLIC ${IMGUI_INCLUDE})
  target_link_libraries(Beams PRIVATE vtkm_rendering vtkm_source)
  # target_link_libraries(Beams PRIVATE glfw GLEW::GLEW OpenGL::GL)
  # target_link_libraries(Beams PRIVATE ${IMGUI_LIB})
  target_link_libraries(Beams PRIVATE tomlplusplus::tomlplusplus)
  target_link_libraries(Beams PRIVATE fmt::fmt)
  target_link_libraries(Beams PRIVATE MPI::MPI_CXX)
  target_link_libraries(Beams 
  PUBLIC Pilot 
  )
  vtkm_add_target_information(Beams
                              DROP_UNUSED_SYMBOLS
                              MODIFY_CUDA_FLAGS
                              DEVICE_SOURCES ${BEAMS_SOURCES})
endif()