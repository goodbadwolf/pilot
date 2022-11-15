################################
# Setup OpenGL
################################
set (OpenGL_GL_PREFERENCE "GLVND")
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
set(OPENGL_INCLUDE_DIR ${OPENGL_INCLUDE_DIR}/GL)
# set(OPENGL_gl_LIRBARY ${OPENGL_INCLUDE_DIR}/Libraries/libGL.dylib)

blt_register_library(NAME opengl
    INCLUDES ${OPENGL_INCLUDE_DIR}
    LIBRARIES ${OPENGL_LIBRARY})

message("Manish ${GLEW_LIBRARIES}")
blt_register_library(NAME glew
   INCLUDES ${GLEW_INCLUDE_DIRS}
   LIBRARIES ${GLEW_LIBRARIES})

