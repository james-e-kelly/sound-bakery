message(STATUS "Adding editor")

set(BUILD_FOLDER ${CMAKE_BINARY_DIR})

set(EDITOR_SOURCES
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

target_link_libraries(editor
    PRIVATE
    SoundBakery::Gluten
    SoundBakery::SoundBakery
)

if(MSVC)
  target_compile_options(editor PRIVATE /W4)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
else()
  target_compile_options(editor PRIVATE -Wall -Wextra -Wpedantic)
endif()

set(CMAKE_FOLDER apps)

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

install(TARGETS editor DESTINATION apps)