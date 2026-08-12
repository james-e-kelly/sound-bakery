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
    SoundBakery::SoundBakery_Shared
)

target_compile_options(game PRIVATE ${PROJECT_WARNING_FLAGS})

set(CMAKE_FOLDER apps)

sbk_add_format_target(NAME game
    SOURCES ${GAME_SOURCES} ${GAME_HEADERS} ${PCH_HEADER} ${PCH_SOURCE})

if (SOUND_BAKERY_INSTALL)
sbk_install(TARGETS game DESTINATION apps/${SBK_INSTALL_CONFIG_SUBDIR})
endif()