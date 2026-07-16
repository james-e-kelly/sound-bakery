CPMAddPackage(
    NAME imgui
    GITHUB_REPOSITORY ocornut/imgui
    GIT_TAG v1.92.7-docking
    DOWNLOAD_ONLY TRUE
)

CPMAddPackage(
    NAME implot
    GITHUB_REPOSITORY epezent/implot
    GIT_TAG v1.0
    DOWNLOAD_ONLY TRUE
)

CPMAddPackage(
  NAME imguizmo
  GITHUB_REPOSITORY CedricGuillemet/ImGuizmo
  GIT_TAG 1.9
  DOWNLOAD_ONLY TRUE
)

CPMAddPackage(
  NAME imspinner
  GITHUB_REPOSITORY dalerank/imspinner
  GIT_TAG master
  DOWNLOAD_ONLY TRUE
)

CPMAddPackage(
  NAME glfw
  GITHUB_REPOSITORY TheCherno/glfw
  GIT_TAG master
)

message("PRINTING IMGUI DIR")
message(${imgui_SOURCE_DIR})

add_library(imgui STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
  ${implot_SOURCE_DIR}/implot.h
  ${implot_SOURCE_DIR}/implot_internal.h
  ${implot_SOURCE_DIR}/implot.cpp
  ${implot_SOURCE_DIR}/implot_items.cpp
  ${implot_SOURCE_DIR}/implot_demo.cpp
  ${imguizmo_SOURCE_DIR}/src/GraphEditor.cpp
  ${imguizmo_SOURCE_DIR}/src/ImCurveEdit.cpp
  ${imguizmo_SOURCE_DIR}/src/ImGradient.cpp
  ${imguizmo_SOURCE_DIR}/src/ImGuizmo.cpp
  ${imguizmo_SOURCE_DIR}/src/ImSequencer.cpp
  ${imspinner_SOURCE_DIR}/imspinner.h
)

c_17(imgui)
cxx_11(imgui)

target_include_directories(imgui
  PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${imgui_SOURCE_DIR}/misc/cpp
    ${implot_SOURCE_DIR}
    ${imguizmo_SOURCE_DIR}
    ${imspinner_SOURCE_DIR}
)

find_package(OpenGL REQUIRED)
set(OpenGL_GL_PREFERENCE "GLVND")

target_link_libraries(imgui PUBLIC
  glfw
  OpenGL::GL
)