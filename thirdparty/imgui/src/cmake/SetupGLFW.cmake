################################
# Setup GLFW3
################################
set(GLFW3_DIR "/usr/lib/x86_64-linux-gnu")
if(NOT GLFW3_DIR)
    MESSAGE(FATAL_ERROR "Rendering needs explicit GLFW3_DIR")
endif()

# set(glfw3_DIR "${GLFW3_DIR}/lib/cmake/glfw3")
set(glfw3_DIR "${GLFW3_DIR}/cmake/glfw3")
find_package(glfw3 REQUIRED)

blt_register_library(NAME glfw
                     INCLUDES /usr/include
                     LIBRARIES glfw)

