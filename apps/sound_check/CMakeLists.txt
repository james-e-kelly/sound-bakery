message(STATUS "Adding Sound Check")

set(BUILD_FOLDER ${CMAKE_BINARY_DIR})

set(SOUND_CHECK_SOURCES
  app/sound_check_app.cpp
  data/workspace_data.cpp
  managers/project_manager.cpp
  managers/review_manager.cpp
  managers/workspace_manager.cpp
  widgets/sound_check_root_widget.cpp
  widgets/intro_widget.cpp
  widgets/workspace_widget.cpp
)

set(SOUND_CHECK_HEADERS
  app/sound_check_app.h
  data/workspace_data.h
  managers/project_manager.h
  managers/review_manager.h
  managers/workspace_manager.h
  widgets/sound_check_root_widget.h
  widgets/intro_widget.h
  widgets/workspace_widget.h
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Source Files" FILES ${SOUND_CHECK_SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Header Files" FILES ${SOUND_CHECK_HEADERS})

set(PCH_HEADER pch.h)
set(PCH_SOURCE pch.cpp)

add_executable(sound_check 
  ${PCH_SOURCE}
  ${SOUND_CHECK_SOURCES}
  ${SOUND_CHECK_HEADERS}
)

set_target_properties(sound_check PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>)

cxx_20(sound_check)

target_include_directories(sound_check
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

target_precompile_headers(sound_check PRIVATE ${PCH_HEADER})

set_target_properties(sound_check PROPERTIES
  OUTPUT_NAME "Sound Check"
)

set_target_properties(sound_check PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER jameskelly.audio
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(sound_check
    PRIVATE
    SoundBakery::Gluten
    SoundBakery::SoundBakery_Shared
)

if(MSVC)
  target_compile_options(sound_check PRIVATE /W4)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
else()
  target_compile_options(sound_check PRIVATE -Wall -Wextra -Wpedantic)
endif()

install(TARGETS sound_check DESTINATION apps)