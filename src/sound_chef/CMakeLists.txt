message(STATUS "Adding Sound Chef")

set(SOUND_CHEF_SOURCES 
    sound_chef.c
    sound_chef_dsp.c
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Source Files" FILES ${ATLAS_SOURCES})

set(SOUND_CHEF_HEADERS
    ${CMAKE_SOURCE_DIR}/inc/sound_chef/sound_chef.h    
    ${CMAKE_SOURCE_DIR}/inc/sound_chef/sound_chef_dsp.h
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Header Files" FILES ${ATLAS_HEADERS})

add_library(sound_chef SHARED ${SOUND_CHEF_SOURCES} ${SOUND_CHEF_HEADERS})
target_include_directories(sound_chef
    PUBLIC ${CMAKE_SOURCE_DIR}/inc/
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

foreach(source IN LISTS SOUND_CHEF_SOURCES)
    list(APPEND SOUND_CHEF_ALL_FILES "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
endforeach()

list(APPEND SOUND_CHEF_ALL_FILES ${SOUND_CHEF_HEADERS})

if (SOUND_BAKERY_FORMAT_SOURCE AND SOUND_BAKERY_CLANG_FORMAT_EXE)
  message(STATUS "Adding Sound Chef clang-format target")
  add_custom_target(format_sound_chef
    COMMAND clang-format --style=file -i ${SOUND_CHEF_ALL_FILES}
    COMMAND_EXPAND_LISTS
    COMMENT "Running clang-format"
    VERBATIM
  )
endif()

# Sad things to make clang-tidy work in VS
# CMakeSettings.json is possible but seemed a pain in its own right
# https://discourse.cmake.org/t/cmake-cxx-clang-tidy-in-msvc/890/9
if(SOUND_BAKERY_TIDY_SOURCE AND SOUND_BAKERY_CLANG_TIDY_EXE AND MSVC)
  set_target_properties(sound_chef PROPERTIES
    VS_GLOBAL_RunCodeAnalysis false

    # Use visual studio core guidelines
    VS_GLOBAL_EnableMicrosoftCodeAnalysis false

    # Use clangtidy
    VS_GLOBAL_EnableClangTidyCodeAnalysis true
  )
endif()

install(TARGETS sound_chef DESTINATION lib)
install(FILES ${SOUND_CHEF_HEADERS} DESTINATION inc)