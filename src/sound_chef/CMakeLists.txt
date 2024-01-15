message(STATUS "Adding Sound Chef")

set(SOUND_CHEF_SOURCES 
    sound_chef.c
    sound_chef_dsp.c
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Source Files" FILES ${ATLAS_SOURCES})

set(SOUND_CHEF_HEADERS
    sound_chef.h    
    sound_chef_dsp.h
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Header Files" FILES ${ATLAS_HEADERS})

add_library(sound_chef STATIC ${SOUND_CHEF_SOURCES} ${SOUND_CHEF_HEADERS})
target_include_directories(sound_chef
    PUBLIC ${CMAKE_SOURCE_DIR}/src/
)

add_library(SoundBakery::SoundChef ALIAS sound_chef)

message(STATUS "Fetching miniaudio")

FetchContent_Declare(
  miniaudio
  GIT_REPOSITORY https://github.com/mackron/miniaudio.git
  GIT_TAG        0.11.21
  GIT_SHALLOW    TRUE
  GIT_PROGRESS TRUE
  OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(miniaudio)

target_include_directories(sound_chef
    PUBLIC ${miniaudio_SOURCE_DIR}
)

if(MSVC)
  target_compile_options(sound_chef PRIVATE /W4)
else()
  target_compile_options(sound_chef PRIVATE -Wall -Wextra -Wpedantic)
endif()

if (SOUND_BAKERY_FORMAT_SOURCE AND SOUND_BAKERY_CLANG_FORMAT_EXE)
  get_target_property(SOUND_CHEF_ALL_FILES sound_chef SOURCES)

  foreach(source IN LISTS SOUND_CHEF_ALL_FILES)
    list(APPEND SOUND_CHEF_FORMAT_FILES "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
  endforeach()

  add_custom_target(format_sound_chef
  COMMAND clang-format --style=file -i ${SOUND_CHEF_FORMAT_FILES}
  COMMAND_EXPAND_LISTS
  COMMENT "Running clang-format"
  VERBATIM
  )
endif()