message(STATUS "Adding Sound Chef")

configure_file (${CMAKE_CURRENT_SOURCE_DIR}/sound_chef_version.h.in ${CMAKE_SOURCE_DIR}/inc/sound_chef/sound_chef_version.h @ONLY)

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
    ${CMAKE_SOURCE_DIR}/inc/sound_chef/sound_chef_version.h
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Header Files" FILES ${ATLAS_HEADERS})

set(CMAKE_FOLDER src)

if (SOUND_BAKERY_SHARED)
  add_library(sound_chef SHARED ${SOUND_CHEF_SOURCES} ${SOUND_CHEF_HEADERS})
else()
  add_library(sound_chef STATIC ${SOUND_CHEF_SOURCES} ${SOUND_CHEF_HEADERS})
endif()

add_library(SoundBakery::SoundChef ALIAS sound_chef)

target_include_directories(sound_chef
    PUBLIC ${CMAKE_SOURCE_DIR}/inc/
    PRIVATE ${CMAKE_SOURCE_DIR}/src/
)

set(CMAKE_FOLDER extern)

message(STATUS "Fetching miniaudio")
FetchContent_MakeAvailable(miniaudio)

message(STATUS "Fetching CLAP")
FetchContent_MakeAvailable(clap)

add_executable(audioconverter ${miniaudio_SOURCE_DIR}/tools/audioconverter/audioconverter.c)
target_include_directories(audioconverter PRIVATE ${miniaudio_SOURCE_DIR})

find_library(MATH_LIBRARY m)
if(MATH_LIBRARY)
    target_link_libraries(audioconverter PUBLIC ${MATH_LIBRARY})
endif()

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# Xiph libraries need C_EXTENSIONS on Linux for using alloca
set(CMAKE_C_EXTENSIONS ON)

message(STATUS "Fetching ogg")
FetchContent_MakeAvailable(ogg)

message(STATUS "Fetching vorbis")
FetchContent_MakeAvailable(vorbis)

message(STATUS "Fetching opus")
FetchContent_MakeAvailable(opus)

message(STATUS "Fetching opusfile")
set(OP_DISABLE_EXAMPLES ON CACHE BOOL "" FORCE)
set(OP_DISABLE_DOCS ON CACHE BOOL "" FORCE)
set(OP_DISABLE_HTTP ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(opusfile)

message(STATUS "Fetching dirent")
FetchContent_MakeAvailable(dirent)

set(CMAKE_C_EXTENSIONS OFF)

target_include_directories(sound_chef
    PUBLIC ${miniaudio_SOURCE_DIR}
    PRIVATE ${opusfile_SOURCE_DIR}/include
    PRIVATE ${dirent_SOURCE_DIR}/include
)

target_link_libraries(sound_chef
    PUBLIC clap
    PRIVATE vorbis
    PRIVATE vorbisfile
    PRIVATE vorbisenc
    PRIVATE Opus::opus
    PRIVATE OpusFile::opusfile)

if(MSVC)
  target_compile_options(sound_chef PRIVATE /W4)
else()
  target_compile_options(sound_chef PRIVATE -Wall -Wextra -Wpedantic)
endif()

get_target_property(SOUND_CHEF_ALL_FILES sound_chef SOURCES)
foreach(source IN LISTS SOUND_CHEF_ALL_FILES)
    list(APPEND SOUND_CHEF_FORMAT_FILES "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
endforeach()

set(CMAKE_FOLDER src)

if (SOUND_BAKERY_FORMAT_SOURCE AND SOUND_BAKERY_CLANG_FORMAT_EXE)
  message(STATUS "Adding Sound Chef clang-format target")
  add_custom_target(format_sound_chef
    COMMAND clang-format --style=file -i ${SOUND_CHEF_FORMAT_FILES}
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
    VS_GLOBAL_RunCodeAnalysis true

    # Use visual studio core guidelines
    VS_GLOBAL_EnableMicrosoftCodeAnalysis false

    # Use clangtidy
    VS_GLOBAL_EnableClangTidyCodeAnalysis true
  )
endif()

install(TARGETS sound_chef DESTINATION lib/sound_chef)
install(FILES ${SOUND_CHEF_HEADERS} DESTINATION inc/sound_chef)
install(FILES ${miniaudio_SOURCE_DIR}/miniaudio.h DESTINATION inc/sound_chef)