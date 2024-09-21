message(STATUS "Adding editor")

set(BUILD_FOLDER ${CMAKE_BINARY_DIR})

set(EDITOR_SOURCES
  app/app.cpp
  managers/app_manager.cpp
  managers/project_manager.cpp
  widgets/imgui_extensions.cpp
  widgets/root_widget.cpp
  widgets/splash_widget.cpp
  widgets/file_browser_widget.cpp
  widgets/play_controls_widget.cpp
  widgets/audio_display_widget.cpp
  widgets/project_explorer_widget.cpp
  widgets/project_nodes_widget.cpp
  widgets/details_widget.cpp
  utils/property_drawer.cpp
  utils/method_drawer.cpp
  widgets/audio_meter_widget.cpp
)

set(EDITOR_HEADERS
  app/app.h
  managers/app_manager.h
  managers/project_manager.h
  widgets/root_widget.h
  widgets/splash_widget.h
  widgets/file_browser_widget.h
  widgets/play_controls_widget.h
  widgets/audio_display_widget.h
  widgets/project_explorer_widget.h
  widgets/project_nodes_widget.h
  widgets/details_widget.h
  utils/property_drawer.h
  utils/method_drawer.h
  widgets/audio_meter_widget.h
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
