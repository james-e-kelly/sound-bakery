message(STATUS "Adding gluten")

message(STATUS "Fetching imspinner")
FetchContent_MakeAvailable(imspinner)

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
  subsystems/widget_subsystem.cpp
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
  subsystems/widget_subsystem.h
  theme/theme.h
  theme/carbon_colors.h
  theme/carbon_theme_g100.h
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

add_library(gluten STATIC
  ${PCH_SOURCE}
  ${GLUTEN_SOURCES}
  ${GLUTEN_HEADERS}
  ${stb_SOURCE_DIR}/stb_image.h
  ${imspinner_SOURCE_DIR}/imspinner.h
)

c_17(gluten)
cxx_20(gluten)

add_library(SoundBakery::Gluten ALIAS gluten)

target_include_directories(gluten
    PUBLIC ${PROJECT_SOURCE_DIR}/src
    PUBLIC ${imspinner_SOURCE_DIR}
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

include(build_nfd)
include(build_imgui)

message(STATUS "Fetching audio fonts")
FetchContent_MakeAvailable(fonts)

message(STATUS "Fetching fmt")
FetchContent_MakeAvailable(fmt)

message(STATUS "Fetching platform folders")
FetchContent_MakeAvailable(platform-folders)

CPMAddPackage("gh:nemtrif/utfcpp@4.0.6")

CPMAddPackage(NAME sodium-cmake
  GITHUB_REPOSITORY robinlinden/libsodium-cmake
  GIT_TAG master
  OPTIONS "SODIUM_DISABLE_TESTS ON"
)

target_include_directories(gluten PUBLIC
  ${fonts_SOURCE_DIR}
)

target_link_libraries(gluten
    PUBLIC
    SoundBakery::SoundBakery_Static
    SoundBakery::Core
    Boost::serialization
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
    utf8cpp
    sodium
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
