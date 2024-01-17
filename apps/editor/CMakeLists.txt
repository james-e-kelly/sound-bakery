message(STATUS "Adding editor")

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

set(EDITOR_SOURCES
  Reflection.cpp
  AppEntry.cpp 
  App/App.cpp
  Managers/AppManager.cpp
  Managers/ProjectManager.cpp
  Subsystems/RendererSubsystem.cpp
  Subsystems/WidgetSubsystem.cpp
  Widgets/Widget.cpp
  Widgets/ImguiExtensions.cpp
  Widgets/RootWidget.cpp
  Widgets/SplashWidget.cpp
  Widgets/FileBrowserWidget.cpp
  Widgets/PlayControlsWidget.cpp
  Widgets/AudioDisplayWidget.cpp
  Widgets/ProjectExplorerWidget.cpp
  Widgets/ProjectNodesWidget.cpp
  Widgets/DetailsWidget.cpp
  Utils/PropertyDrawer.cpp
  Utils/MethodDrawer.cpp
)

set(EDITOR_HEADERS
  App/App.h
  Managers/AppManager.h
  Managers/ProjectManager.h
  Subsystems/RendererSubsystem.h
  Subsystems/WidgetSubsystem.h
  Widgets/Widget.h
  Widgets/RootWidget.h
  Widgets/SplashWidget.h
  Widgets/FileBrowserWidget.h
  Widgets/PlayControlsWidget.h
  Widgets/AudioDisplayWidget.h
  Widgets/ProjectExplorerWidget.h
  Widgets/ProjectNodesWidget.h
  Widgets/DetailsWidget.h
  Utils/PropertyDrawer.h
  Utils/MethodDrawer.h
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Source Files" FILES ${EDITOR_SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Header Files" FILES ${EDITOR_HEADERS})

set(PCH_HEADER pch.h)
set(PCH_SOURCE pch.cpp)

add_executable(editor 
  ${PCH_SOURCE}
  ${EDITOR_SOURCES}
  ${EDITOR_HEADERS}
  ${FONT_FILES}
  ${IMAGE_FILES}
)

target_include_directories(editor
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

target_precompile_headers(editor PRIVATE ${PCH_HEADER})

set_target_properties(editor PROPERTIES
  OUTPUT_NAME "Sound Bakery"
)

set_target_properties(editor PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER jameskelly.audio
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

set_source_files_properties(
  ${FONT_FILES}
  PROPERTIES MACOSX_PACKAGE_LOCATION Resources/fonts
)

set_source_files_properties(
  ${IMAGE_FILES}
  PROPERTIES MACOSX_PACKAGE_LOCATION Resources/images
)

# Set output directory for executable
set_target_properties(editor PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${BUILD_FOLDER})

set(CMAKE_CXX_STANDARD 17) # no need to manually adjust the CXXFLAGS

set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.3.9
  GIT_SHALLOW    TRUE
  OVERRIDE_FIND_PACKAGE
)

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        docking
  GIT_SHALLOW    TRUE
  OVERRIDE_FIND_PACKAGE
)

FetchContent_Declare(
  nfd
  GIT_REPOSITORY https://github.com/mlabbe/nativefiledialog.git
  GIT_TAG        67345b80ebb429ecc2aeda94c478b3bcc5f7888e
  GIT_SHALLOW    TRUE
  OVERRIDE_FIND_PACKAGE
)

FetchContent_Declare(
  fonts
  GIT_REPOSITORY https://github.com/juliettef/IconFontCppHeaders.git
  GIT_TAG        main
  GIT_SHALLOW    TRUE
  OVERRIDE_FIND_PACKAGE
)

message(STATUS "Fetching glfw")
FetchContent_MakeAvailable(glfw)

message(STATUS "Fetching imgui")
FetchContent_MakeAvailable(imgui)

message(STATUS "Fetching native file dialog")
FetchContent_MakeAvailable(nfd)

message(STATUS "Fetching audio fonts")
FetchContent_MakeAvailable(fonts)

if(UNIX AND NOT APPLE)
  set(NFD_SOURCES 
  ${nfd_SOURCE_DIR}/src/nfd_common.c
  ${nfd_SOURCE_DIR}/src/nfd_gtk.c
  ${nfd_SOURCE_DIR}/src/nfd_zenity.c
  )

elseif(APPLE)
  set(NFD_SOURCES 
  ${nfd_SOURCE_DIR}/src/nfd_common.c
  ${nfd_SOURCE_DIR}/src/nfd_cocoa.m
  )
elseif(WIN32)
  set(NFD_SOURCES 
  ${nfd_SOURCE_DIR}/src/nfd_common.c
  ${nfd_SOURCE_DIR}/src/nfd_win.cpp
  )
endif()

add_library(nfd STATIC ${NFD_SOURCES})

if(APPLE)
  find_library(APPKIT_FRAMEWORK AppKit)
  target_link_libraries(nfd ${APPKIT_FRAMEWORK})
endif()

target_include_directories(nfd
  PUBLIC ${nfd_SOURCE_DIR}/src/include
  PRIVATE ${nfd_SOURCE_DIR}/src
)

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
)

target_include_directories(imgui
  PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(imgui
  glfw
  OpenGL::GL
)

target_include_directories(editor PRIVATE
  ${fonts_SOURCE_DIR}
)

target_link_libraries(editor
    SoundBakery::SoundBakery
    nfd
    glfw
    imgui
)

if(MSVC)
  target_compile_options(editor PRIVATE /W4)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
else()
  target_compile_options(editor PRIVATE -Wall -Wextra -Wpedantic)
endif()

if (SOUND_BAKERY_FORMAT_SOURCE AND SOUND_BAKERY_CLANG_FORMAT_EXE)
  get_target_property(SOUND_BAKERY_EDITOR_ALL_FILES editor SOURCES)

  foreach(source IN LISTS EDITOR_SOURCES EDITOR_HEADERS PCH_HEADER PCH_SOURCE)
    list(APPEND SOUND_BAKERY_EDITOR_FORMAT_FILES "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
  endforeach()

  add_custom_target(format_editor
  COMMAND clang-format --style=file -i ${SOUND_BAKERY_EDITOR_FORMAT_FILES}
  COMMAND_EXPAND_LISTS
  COMMENT "Running clang-format"
  VERBATIM
  )
endif()