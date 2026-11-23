message(STATUS "Adding Viewer")

set(BUILD_FOLDER ${CMAKE_BINARY_DIR})

set(viewer_SOURCES
  app/viewer_app.cpp
  widgets/viewer_root_widget.cpp
)

set(viewer_HEADERS
  app/viewer_app.h
  widgets/viewer_root_widget.h
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Source Files" FILES ${viewer_SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Header Files" FILES ${viewer_HEADERS})

set(PCH_HEADER pch.h)
set(PCH_SOURCE pch.cpp)

add_executable(viewer
  ${PCH_SOURCE}
  ${viewer_SOURCES}
  ${viewer_HEADERS}
)

cxx_20(viewer)

target_include_directories(viewer
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_precompile_headers(viewer PRIVATE ${PCH_HEADER})

set_target_properties(viewer PROPERTIES
  OUTPUT_NAME "Sound Viewer"
)

set_target_properties(viewer PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER jameskelly.audio
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(viewer
    PRIVATE
    SoundBakery::Gluten
)
target_compile_options(viewer PRIVATE ${PROJECT_WARNING_FLAGS})

set(CMAKE_FOLDER apps)

sbk_add_format_target(NAME viewer
    SOURCES ${viewer_SOURCES} ${viewer_HEADERS} ${PCH_HEADER} ${PCH_SOURCE})

if (SOUND_BAKERY_INSTALL)
sbk_install(TARGETS viewer DESTINATION apps/${SBK_INSTALL_CONFIG_SUBDIR})
endif()