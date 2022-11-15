########################################
# main imgui demo
########################################
set(imgui_demo_sources imgui_demo.cpp)

blt_add_executable(
    NAME       imgui_demo
    SOURCES    ${imgui_demo_sources}
    DEPENDS_ON imgui)

# set(implot_demo_sources implot_demo.cpp)
#
# blt_add_executable(
#     NAME       implot_demo
#     SOURCES    ${implot_demo_sources}
#     DEPENDS_ON imgui)
