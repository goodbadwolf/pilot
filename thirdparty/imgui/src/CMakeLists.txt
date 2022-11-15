cmake_minimum_required(VERSION 3.7)
################################
# cmake policy selections
################################
# avoid default mac osx rpath settings for cmake 3.0
cmake_policy(SET CMP0042 OLD)
# avoid warnings for project commands w/o VERSION
cmake_policy(SET CMP0048 NEW)

project(imgui_cmake VERSION "0.0.1")

################################
# Setup BLT
################################
if(NOT BLT_CXX_STD)
  set(BLT_CXX_STD "c++14" CACHE STRING "")
endif()

include(./blt/SetupBLT.cmake)
include(./cmake/SetupOpenGL.cmake)
include(./cmake/SetupGLFW.cmake)

#######################################
# Compile the IMGUI library
#######################################
set(imgui_sources imgui/imgui.cpp
                  imgui/imgui_demo.cpp
                  imgui/imgui_draw.cpp
                  imgui/imgui_tables.cpp
                  imgui/imgui_widgets.cpp
                  imgui/backends/imgui_impl_glfw.cpp
                  imgui/backends/imgui_impl_opengl3.cpp)

set(imgui_headers imgui/imconfig.h
                  imgui/imgui.h
                  imgui/backends/imgui_impl_glfw.h
                  imgui/backends/imgui_impl_opengl3.h)

# Some of these add-ons expect to be in the imgui directory
# so we will just add the includes
include_directories(imgui)
#######################################
# implot
#######################################
# set(implot_sources implot/implot.cpp
#                    implot/implot_demo.cpp
#                    implot/implot_items.cpp)

# set(implot_headers implot/implot.h)

# list(APPEND imgui_sources ${implot_sources})
# list(APPEND imgui_headers ${implot_headers})

# include_directories(implot)

#######################################
# imgui node editor
#######################################
set(node_editor_sources imgui-node-editor/imgui_canvas.cpp
                        imgui-node-editor/crude_json.cpp
                        imgui-node-editor/imgui_node_editor.cpp
                        imgui-node-editor/imgui_node_editor_api.cpp)

set(node_editor_headers imgui-node-editor/imgui_node_editor.h)

list(APPEND imgui_sources ${node_editor_sources})
list(APPEND imgui_headers ${node_editor_headers})

include_directories(imgui-node-editor)

blt_add_library(
    NAME       imgui
    SOURCES    ${imgui_sources}
    HEADERS    ${imgui_headers}
    DEPENDS_ON opengl glfw glew
    SHARED     TRUE)

install(TARGETS imgui
        EXPORT  imgui_cmake
        LIBRARY DESTINATION lib/cmake/${PROJECT_NAME})
install (FILES ${imgui_headers} DESTINATION include/)

# add_subdirectory(examples)
