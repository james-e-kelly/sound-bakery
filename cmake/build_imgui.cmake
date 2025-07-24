message(STATUS "Fetching imgui")
FetchContent_MakeAvailable(imgui)

message(STATUS "Fetching implot")
FetchContent_MakeAvailable(implot)

CPMAddPackage(
  NAME imguizmo
  GITHUB_REPOSITORY CedricGuillemet/ImGuizmo
  GIT_TAG master
  EXCLUDE_FROM_ALL
)

CPMAddPackage(
  NAME imguitextselect
  GITHUB_REPOSITORY AidanSun05/ImGuiTextSelect
  GIT_TAG main
  EXCLUDE_FROM_ALL
)

CPMAddPackage("gh:nemtrif/utfcpp@4.0.6")

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
  ${imguizmo_SOURCE_DIR}/GraphEditor.cpp
  ${imguizmo_SOURCE_DIR}/ImCurveEdit.cpp
  ${imguizmo_SOURCE_DIR}/ImGradient.cpp
  ${imguizmo_SOURCE_DIR}/ImGuizmo.cpp
  ${imguizmo_SOURCE_DIR}/ImSequencer.cpp
  ${imguitextselect_SOURCE_DIR}/textselect.cpp
  ${imguitextselect_SOURCE_DIR}/textselect.hpp
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
    ${imguitextselect_SOURCE_DIR}
)

find_package(OpenGL REQUIRED)
set(OpenGL_GL_PREFERENCE "GLVND")

message(STATUS "Fetching glfw")
FetchContent_MakeAvailable(glfw)

target_link_libraries(imgui PUBLIC
  glfw
  OpenGL::GL
  utf8cpp
)