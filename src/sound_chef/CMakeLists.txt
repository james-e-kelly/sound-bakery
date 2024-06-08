message(STATUS "Adding Sound Chef")

set(SOUND_CHEF_SOURCES 
    sound_chef.c
    sound_chef_bank.c
    sound_chef_dsp.c
    sound_chef_encoder.c

    encoders/sound_chef_encoder_vorbis.c
    encoders/vorbis_encode_test.c
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Source Files" FILES ${ATLAS_SOURCES})

set(SOUND_CHEF_HEADERS
    sound_chef_bank.h
    sound_chef_encoder.h
    sound_chef_guide.h

    encoders/sound_chef_encoder_vorbis.h

    ${CMAKE_SOURCE_DIR}/inc/sound_chef/sound_chef.h    
    ${CMAKE_SOURCE_DIR}/inc/sound_chef/sound_chef_dsp.h
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Header Files" FILES ${ATLAS_HEADERS})

add_library(sound_chef_shared SHARED ${SOUND_CHEF_SOURCES} ${SOUND_CHEF_HEADERS})
add_library(sound_chef_static STATIC ${SOUND_CHEF_SOURCES} ${SOUND_CHEF_HEADERS})

target_include_directories(sound_chef_shared
    PUBLIC ${CMAKE_SOURCE_DIR}/inc/
    PRIVATE ${CMAKE_SOURCE_DIR}/src/
)

target_include_directories(sound_chef_static
    PUBLIC ${CMAKE_SOURCE_DIR}/inc/
    PRIVATE ${CMAKE_SOURCE_DIR}/src/
)

add_library(SoundBakery::SoundChef_Shared ALIAS sound_chef_shared)
add_library(SoundBakery::SoundChef_Static ALIAS sound_chef_static)

set(CMAKE_FOLDER extern)

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

add_executable(audioconverter ${miniaudio_SOURCE_DIR}/tools/audioconverter/audioconverter.c)
target_include_directories(audioconverter PRIVATE ${miniaudio_SOURCE_DIR})

find_library(MATH_LIBRARY m)
if(MATH_LIBRARY)
    target_link_libraries(audioconverter PUBLIC ${MATH_LIBRARY})
endif()

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

message(STATUS "Fetching ogg")

FetchContent_Declare(
  ogg
  GIT_REPOSITORY https://github.com/xiph/ogg.git
  GIT_TAG        v1.3.5
  GIT_SHALLOW    TRUE
  GIT_PROGRESS TRUE
  OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(ogg)

message(STATUS "Fetching vorbis")

FetchContent_Declare(
  vorbis
  GIT_REPOSITORY https://github.com/xiph/vorbis.git
  GIT_TAG        v1.3.7
  GIT_SHALLOW    TRUE
  GIT_PROGRESS TRUE
  OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(vorbis)

message(STATUS "Fetching opus")

FetchContent_Declare(
  opus
  GIT_REPOSITORY https://github.com/xiph/opus.git
  GIT_TAG        v1.5.2
  GIT_SHALLOW    TRUE
  GIT_PROGRESS TRUE
  OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(opus)

message(STATUS "Fetching opusfile")

set(OP_DISABLE_EXAMPLES ON CACHE BOOL "" FORCE)
set(OP_DISABLE_DOCS ON CACHE BOOL "" FORCE)
set(OP_DISABLE_HTTP ON CACHE BOOL "" FORCE)

FetchContent_Declare(
  opusfile
  GIT_REPOSITORY https://github.com/KarateKidzz/opusfile.git
  GIT_TAG        master
  GIT_SHALLOW    TRUE
  GIT_PROGRESS TRUE
  OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(opusfile)

target_include_directories(sound_chef_shared
    PUBLIC ${miniaudio_SOURCE_DIR}
    PRIVATE ${opusfile_SOURCE_DIR}/include
)

target_include_directories(sound_chef_static
    PUBLIC ${miniaudio_SOURCE_DIR}
    PRIVATE ${opusfile_SOURCE_DIR}/include
)

target_link_libraries(sound_chef_shared
    PRIVATE vorbis
    PRIVATE vorbisfile
    PRIVATE vorbisenc
    PRIVATE Opus::opus
    PRIVATE OpusFile::opusfile)
target_link_libraries(sound_chef_static
    PRIVATE vorbis
    PRIVATE vorbisfile
    PRIVATE vorbisenc
    PRIVATE Opus::opus
    PRIVATE OpusFile::opusfile)

if(MSVC)
  target_compile_options(sound_chef_shared PRIVATE /W4)
  target_compile_options(sound_chef_static PRIVATE /W4)
else()
  target_compile_options(sound_chef_shared PRIVATE -Wall -Wextra -Wpedantic)
  target_compile_options(sound_chef_static PRIVATE -Wall -Wextra -Wpedantic)
endif()

foreach(source IN LISTS SOUND_CHEF_SOURCES)
    list(APPEND SOUND_CHEF_ALL_FILES "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
endforeach()

list(APPEND SOUND_CHEF_ALL_FILES ${SOUND_CHEF_HEADERS})

set(CMAKE_FOLDER src)

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
  set_target_properties(sound_chef_shared PROPERTIES
    VS_GLOBAL_RunCodeAnalysis true

    # Use visual studio core guidelines
    VS_GLOBAL_EnableMicrosoftCodeAnalysis false

    # Use clangtidy
    VS_GLOBAL_EnableClangTidyCodeAnalysis true
  )

  set_target_properties(sound_chef_static PROPERTIES
    VS_GLOBAL_RunCodeAnalysis true

    # Use visual studio core guidelines
    VS_GLOBAL_EnableMicrosoftCodeAnalysis false

    # Use clangtidy
    VS_GLOBAL_EnableClangTidyCodeAnalysis true
  )
endif()

install(TARGETS sound_chef_shared DESTINATION lib)
install(FILES ${SOUND_CHEF_HEADERS} DESTINATION inc)