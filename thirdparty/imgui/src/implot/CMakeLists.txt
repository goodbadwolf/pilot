set(implot_sources implot.cpp
                   implot_demo.cpp
                   implot_items.cpp)
set(implot_headers implot.h)

blt_add_library(
    NAME       implot
    SOURCES    ${implot_sources}
    HEADERS    ${implot_headers}
    DEPENDS_ON imgui)
