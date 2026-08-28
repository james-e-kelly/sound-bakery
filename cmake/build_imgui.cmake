include_guard(GLOBAL)

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
  GIT_TAG ffe57a9cf741a92bdb6042cd4f8eb152b9c95b1d # master @ 2026-07-17
  DOWNLOAD_ONLY TRUE
)

CPMAddPackage(
  NAME glfw
  GITHUB_REPOSITORY TheCherno/glfw
  GIT_TAG 026a148d7dd78d597de380c4e77ca0869f0ceaab # master @ 2026-07-17
)

# FreeType powers imgui's font rasterizer (misc/freetype/imgui_freetype.cpp).
# Enabled by defining IMGUI_ENABLE_FREETYPE + compiling that TU + linking freetype;
# no runtime code changes needed on imgui 1.92's font loader API. The optional
# format handlers (png/zlib/etc.) aren't needed for TTF rasterization, so disable
# them to keep the dependency small and avoid pulling extra transitives.
CPMAddPackage(
  NAME freetype
  GITHUB_REPOSITORY freetype/freetype
  GIT_TAG VER-2-13-3
  OPTIONS
    "FT_DISABLE_HARFBUZZ ON"
    "FT_DISABLE_BROTLI ON"
    "FT_DISABLE_BZIP2 ON"
    "FT_DISABLE_PNG ON"
    "FT_DISABLE_ZLIB ON"
    "FT_DISABLE_ZSTD ON"
    "SKIP_INSTALL_ALL ON"
)

add_library(imgui STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
  ${imgui_SOURCE_DIR}/misc/freetype/imgui_freetype.cpp
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

# PUBLIC so every TU that includes imgui.h agrees the freetype loader is present.
target_compile_definitions(imgui PUBLIC IMGUI_ENABLE_FREETYPE)

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

set(OpenGL_GL_PREFERENCE "GLVND")
find_package(OpenGL REQUIRED)

target_link_libraries(imgui PUBLIC
  glfw
  OpenGL::GL
  freetype
)