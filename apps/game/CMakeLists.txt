message(STATUS "Adding fake game")

set(BUILD_FOLDER ${CMAKE_BINARY_DIR})

set(GAME_SOURCES
  app/app.cpp
  widgets/root_widget.cpp
  widgets/soundbank_viewer.cpp
)

set(GAME_HEADERS
  app/app.h
  widgets/root_widget.h
  widgets/soundbank_viewer.h
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Source Files" FILES ${GAME_SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Header Files" FILES ${GAME_HEADERS})

set(PCH_HEADER pch.h)
set(PCH_SOURCE pch.cpp)

add_executable(game 
  ${PCH_SOURCE}
  ${GAME_SOURCES}
  ${GAME_HEADERS}
)

cxx_20(game)

target_include_directories(game
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

target_precompile_headers(game PRIVATE ${PCH_HEADER})

set_target_properties(game PROPERTIES
  OUTPUT_NAME "Sound Bakery Test Game"
)

set_target_properties(game PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER jameskelly.audio
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(game
    PRIVATE
    SoundBakery::Gluten
    SoundBakery::SoundBakery
)

if(MSVC)
  target_compile_options(game PRIVATE /W4)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
else()
  target_compile_options(game PRIVATE -Wall -Wextra -Wpedantic)
endif()

set(CMAKE_FOLDER apps)

if (SOUND_BAKERY_FORMAT_SOURCE AND SOUND_BAKERY_CLANG_FORMAT_EXE)
  get_target_property(SOUND_BAKERY_GAME_ALL_FILES game SOURCES)

  foreach(source IN LISTS GAME_SOURCES GAME_HEADERS PCH_HEADER PCH_SOURCE)
    list(APPEND SOUND_BAKERY_GAME_FORMAT_FILES "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
  endforeach()

  add_custom_target(format_game
  COMMAND clang-format --style=file -i ${SOUND_BAKERY_EDITOR_FORMAT_FILES}
  COMMAND_EXPAND_LISTS
  COMMENT "Running clang-format"
  VERBATIM
  )
endif()

install(TARGETS editor DESTINATION apps)