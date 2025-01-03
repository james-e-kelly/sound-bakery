message(STATUS "Adding gluten")

set(BUILD_FOLDER ${CMAKE_BINARY_DIR})

set(FONT_FILES
  ${BUILD_FOLDER}/fonts/Montserrat-Light.ttf
  ${BUILD_FOLDER}/fonts/fa-regular-400.ttf
  ${BUILD_FOLDER}/fonts/fa-solid-900.ttf
  ${BUILD_FOLDER}/fonts/fontaudio.ttf
)

set(IMAGE_FILES
  ${BUILD_FOLDER}/images/SplashImage.png
)

set(GLUTEN_SOURCES
  app_entry.cpp 
  app/app.cpp
  elements/element.cpp
  elements/button.cpp
  elements/image_button.cpp
  elements/text.cpp
  elements/image.cpp
  elements/slider.cpp
  elements/menu_bar.cpp
  elements/toolbar.cpp
  elements/layouts/layout.cpp
  managers/manager.cpp
  subsystems/renderer_subsystem.cpp
  subsystems/widget_subsystem.cpp
  utils/imgui_util_functions.cpp
  theme/window_images.embed
  theme/walnut_icon.embed
  widgets/widget.cpp
  widgets/root_widget.cpp
)

set(GLUTEN_HEADERS
  app/app.h
  elements/element.h
  elements/button.h
  elements/image_button.h
  elements/text.h
  elements/image.h
  elements/slider.h
  elements/menu_bar.h
  elements/toolbar.h
  elements/layouts/layout.h
  managers/manager.h
  subsystems/renderer_subsystem.h
  subsystems/widget_subsystem.h
  theme/theme.h
  theme/carbon_colors.h
  theme/carbon_theme_g100.h
  utils/imgui_util_functions.h
  utils/imgui_util_structures.h
  widgets/widget.h
  widgets/root_widget.h
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Source Files" FILES ${GLUTEN_SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Header Files" FILES ${GLUTEN_HEADERS})

set(PCH_HEADER pch.h)
set(PCH_SOURCE pch.cpp)

add_library(gluten STATIC
  ${PCH_SOURCE}
  ${GLUTEN_SOURCES}
  ${GLUTEN_HEADERS}
  ${stb_SOURCE_DIR}/stb_image.h
)

add_library(SoundBakery::Gluten ALIAS gluten)

target_include_directories(gluten
    PUBLIC ${CMAKE_SOURCE_DIR}/src
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE ${stb_SOURCE_DIR}
)

target_precompile_headers(gluten PRIVATE ${PCH_HEADER})

set_source_files_properties(
  ${FONT_FILES}
  PROPERTIES MACOSX_PACKAGE_LOCATION Resources/fonts
)

set_source_files_properties(
  ${IMAGE_FILES}
  PROPERTIES MACOSX_PACKAGE_LOCATION Resources/images
)

set(CMAKE_FOLDER extern)

set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

message(STATUS "Fetching bytesize")
FetchContent_MakeAvailable(bytesizelib)
add_library(bytesize INTERFACE)
target_include_directories(bytesize INTERFACE ${bytesizelib_SOURCE_DIR})

message(STATUS "Fetching CMRC for resource embedding")
FetchContent_MakeAvailable(cmrc)

message(STATUS "Fetching glfw")
FetchContent_MakeAvailable(glfw)

message(STATUS "Fetching imgui")
FetchContent_MakeAvailable(imgui)

message(STATUS "Fetching implot")
FetchContent_MakeAvailable(implot)

include(build_nfd)

message(STATUS "Fetching audio fonts")
FetchContent_MakeAvailable(fonts)

message(STATUS "Fetching fmt")
FetchContent_MakeAvailable(fmt)

find_package(OpenGL REQUIRED)
set(OpenGL_GL_PREFERENCE "GLVND")

add_library(imgui STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${implot_SOURCE_DIR}/implot.h
  ${implot_SOURCE_DIR}/implot_internal.h
  ${implot_SOURCE_DIR}/implot.cpp
  ${implot_SOURCE_DIR}/implot_items.cpp
  ${implot_SOURCE_DIR}/implot_demo.cpp
)

target_include_directories(imgui
  PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${implot_SOURCE_DIR}
)

target_link_libraries(imgui PUBLIC
  glfw
  OpenGL::GL
)

target_include_directories(gluten PUBLIC
  ${fonts_SOURCE_DIR}
)

target_link_libraries(gluten
    PUBLIC
    SoundBakery::Delegates 
    SoundBakery::Fonts
    SoundBakery::Images
    sbk::icon_images
    nfd
    glfw
    imgui
    fmt::fmt
    bytesize
    Tracy::TracyClient
)

if(MSVC)
  target_compile_options(gluten PRIVATE /W4)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:__cplusplus")
else()
  target_compile_options(gluten PRIVATE -Wall -Wextra -Wpedantic)
endif()

set(CMAKE_FOLDER src)

if (SOUND_BAKERY_FORMAT_SOURCE AND SOUND_BAKERY_CLANG_FORMAT_EXE)
  get_target_property(SOUND_BAKERY_GLUTEN_ALL_FILES gluten SOURCES)

  foreach(source IN LISTS GLUTEN_SOURCES GLUTEN_HEADERS PCH_HEADER PCH_SOURCE)
    list(APPEND SOUND_BAKERY_GLUTEN_FORMAT_FILES "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
  endforeach()

  add_custom_target(format_gluten
  COMMAND clang-format --style=file -i ${SOUND_BAKERY_GLUTEN_FORMAT_FILES}
  COMMAND_EXPAND_LISTS
  COMMENT "Running clang-format"
  VERBATIM
  )
endif()