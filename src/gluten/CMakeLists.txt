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
  data/data_source.cpp
  data/data_cache.cpp
  elements/element.cpp
  elements/audio_element.cpp
  elements/file_element.cpp
  elements/video_element.cpp
  elements/button.cpp
  elements/image_button.cpp
  elements/text.cpp
  elements/image.cpp
  elements/slider.cpp
  elements/menu_bar.cpp
  elements/toolbar.cpp
  elements/icon_button.cpp
  elements/collapsing_header.cpp
  elements/loading_spinner.cpp
  elements/layouts/layout.cpp
  managers/manager.cpp
  subsystems/audio_subsystem.cpp
  subsystems/renderer_subsystem.cpp
  subsystems/video_subsystem.cpp
  subsystems/widget_subsystem.cpp
  glad/src/gl.cpp
  theme/theme.cpp
  utils/imgui_util_functions.cpp
  theme/window_images.embed
  theme/walnut_icon.embed
  widgets/widget.cpp
  widgets/root_widget.cpp
  widgets/popup_widget.cpp
  widgets/window_widget.cpp
)

set(GLUTEN_HEADERS
  app/app.h
  data/data_source.h
  data/data_controller.h
  data/data_cache.h
  elements/element.h
  elements/audio_element.h
  elements/file_element.h
  elements/video_element.h
  elements/button.h
  elements/image_button.h
  elements/text.h
  elements/image.h
  elements/slider.h
  elements/menu_bar.h
  elements/toolbar.h
  elements/icon_button.h
  elements/collapsing_header.h
  elements/loading_spinner.h
  elements/layouts/layout.h
  managers/manager.h
  subsystems/audio_subsystem.h
  subsystems/renderer_subsystem.h
  subsystems/video_subsystem.h
  subsystems/widget_subsystem.h
  glad/include/glad/gl.h
  glad/include/KHR/khrplatform.h
  theme/theme.h
  theme/carbon/carbon_colors.h
  theme/carbon/carbon_theme_g100.h
  theme/things/things.h
  utils/imgui_util_functions.h
  utils/imgui_util_structures.h
  utils/boost_serialization_extensions.h
  widgets/widget.h
  widgets/root_widget.h
  widgets/popup_widget.h
  widgets/window_widget.h
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Source Files" FILES ${GLUTEN_SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Header Files" FILES ${GLUTEN_HEADERS})

set(PCH_HEADER pch.h)
set(PCH_SOURCE pch.cpp)

include(build_mpv)

add_library(gluten STATIC
  ${PCH_SOURCE}
  ${GLUTEN_SOURCES}
  ${GLUTEN_HEADERS}
  ${stb_SOURCE_DIR}/stb_image.h
)

c_17(gluten)
cxx_20(gluten)

add_library(SoundBakery::Gluten ALIAS gluten)

target_include_directories(gluten
    PUBLIC ${PROJECT_SOURCE_DIR}/src
    PUBLIC ${MPV_INCLUDE_DIRS}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/glad/include
    PRIVATE ${stb_SOURCE_DIR}
)

# GLUTEN_ENABLE_VIDEO gates the mpv-backed video subsystem. PUBLIC so any app
# including gluten's video_subsystem.h (directly or transitively) compiles the
# same variant (real subsystem vs. no-op stub) and stays ABI-consistent.
if(GLUTEN_ENABLE_VIDEO)
  target_compile_definitions(gluten PUBLIC GLUTEN_ENABLE_VIDEO=1)

  # mpv/client.h declares its functions as extern function pointers when this is set, matching
  # video_subsystem's dlopen/dlsym-based loading. PUBLIC so any app including gluten's video_subsystem.h
  # (directly or transitively) sees the same declarations.
  target_compile_definitions(gluten PUBLIC "MPV_CPLUGIN_DYNAMIC_SYM")
else()
  target_compile_definitions(gluten PUBLIC GLUTEN_ENABLE_VIDEO=0)
endif()

if(USE_MPV_WIN_BUILD)
  add_dependencies(gluten mpv_dev)
endif()

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

CPMAddPackage(
  NAME bytesizelib
  GITHUB_REPOSITORY eudoxos/bytesize
  GIT_TAG 196542fed6b4234be6994eeb43984e3a3ba86b95 # master @ 2026-07-17
  DOWNLOAD_ONLY YES
)
add_library(bytesize INTERFACE)
target_include_directories(bytesize INTERFACE ${bytesizelib_SOURCE_DIR})

include(build_nfd)

CPMAddPackage(
  NAME fonts
  GITHUB_REPOSITORY juliettef/IconFontCppHeaders
  GIT_TAG bbe9ecd24203c4de10d1b9e3fdeb4edf5c6cb842
  DOWNLOAD_ONLY YES
)

CPMAddPackage(
  NAME fmt
  GITHUB_REPOSITORY fmtlib/fmt
  GIT_TAG 12.2.0
  EXCLUDE_FROM_ALL YES
)

CPMAddPackage(
  NAME platform-folders
  GITHUB_REPOSITORY sago007/PlatformFolders
  GIT_TAG f2625faed48bc891eb624187f8a0e4a1d1cd6ad6 # master @ 2026-07-17
  EXCLUDE_FROM_ALL YES
)

include(build_imgui)

# libsodium and libebur128 each install() their own headers/libs/config;
# suppressed by default like every dependency (see setup_install_suppression.cmake).
CPMAddPackage(NAME sodium-cmake
  GITHUB_REPOSITORY robinlinden/libsodium-cmake
  GIT_TAG 9b2848dfc1b917a9410f0de9d81059b26cbfaa8d # master @ 2026-07-17
  OPTIONS "SODIUM_DISABLE_TESTS ON"
)

CPMAddPackage(NAME libebur128
  GITHUB_REPOSITORY jiixyj/libebur128
  GIT_TAG 67b33abe1558160ed76ada1322329b0e9e058b02 # master @ 2026-07-17
  OPTIONS "ENABLE_TESTS OFF" "BUILD_SHARED_LIBS OFF"
)

target_include_directories(gluten PUBLIC
  ${fonts_SOURCE_DIR}
  ${libebur128_SOURCE_DIR}
)

target_link_libraries(gluten
    PUBLIC
    SoundBakery::SoundBakery_Static
    SoundBakery::Core
    Boost::serialization
    Boost::program_options
    concurrencpp::concurrencpp 
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
    RTTR::Core_Lib
    sago::platform_folders
    sodium
    ebur128
)

target_compile_options(gluten PRIVATE ${PROJECT_WARNING_FLAGS})

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
