message(STATUS "Adding Sound Chef")

configure_file (${CMAKE_CURRENT_SOURCE_DIR}/sound_chef_version.h.in ${PROJECT_SOURCE_DIR}/inc/sound_chef/sound_chef_version.h @ONLY)

set(SOUND_CHEF_SOURCES 
    sound_chef.c
    sound_chef_bank.c
    sound_chef_dsp.c
    sound_chef_encoder.c
    sound_chef_internal.c

    dsp/sound_chef_dsp_clap.c
    dsp/sound_chef_dsp_delay.c
    dsp/sound_chef_dsp_fader.c
    dsp/sound_chef_dsp_highpass.c
    dsp/sound_chef_dsp_lowpass.c
    dsp/sound_chef_dsp_meter.c

    encoders/sound_chef_encoder_vorbis.c
    encoders/vorbis_encode_test.c
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Source Files" FILES ${SOUND_CHEF_SOURCES})

set(SOUND_CHEF_LOCAL_HEADERS
    sound_chef_bank.h
    sound_chef_encoder.h
    sound_chef_guide.h
    sound_chef_internal.h

    encoders/sound_chef_encoder_vorbis.h
)

set(SOUND_CHEF_PUBLIC_HEADERS
    ${PROJECT_SOURCE_DIR}/inc/sound_chef/sound_chef.h
    ${PROJECT_SOURCE_DIR}/inc/sound_chef/sound_chef_common.h
    ${PROJECT_SOURCE_DIR}/inc/sound_chef/sound_chef_dsp.h
    ${PROJECT_SOURCE_DIR}/inc/sound_chef/sound_chef_version.h
)

set(SOUND_CHEF_HEADERS ${SOUND_CHEF_LOCAL_HEADERS} ${SOUND_CHEF_PUBLIC_HEADERS})

# Local and public headers live under different roots (src/sound_chef vs inc/sound_chef), so
# source_group(TREE ...) - which requires a single common root - needs to be called once per root.
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Header Files" FILES ${SOUND_CHEF_LOCAL_HEADERS})
source_group(TREE ${PROJECT_SOURCE_DIR}/inc/sound_chef PREFIX "Header Files" FILES ${SOUND_CHEF_PUBLIC_HEADERS})

set(CMAKE_FOLDER src)

add_library(sound_chef_shared SHARED ${SOUND_CHEF_SOURCES} ${SOUND_CHEF_HEADERS} ${stb_SOURCE_DIR}/stb_ds.h)
add_library(sound_chef_static STATIC ${SOUND_CHEF_SOURCES} ${SOUND_CHEF_HEADERS} ${stb_SOURCE_DIR}/stb_ds.h)

c_17(sound_chef_shared)
c_17(sound_chef_static)
cxx_20(sound_chef_shared)
cxx_20(sound_chef_static)

set_target_properties(sound_chef_shared PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>)

target_compile_options(sound_chef_shared PRIVATE ${PROJECT_LIBRARY_WARNING_FLAGS})
target_compile_options(sound_chef_static PRIVATE ${PROJECT_LIBRARY_WARNING_FLAGS})

add_library(SoundBakery::SoundChef_Shared ALIAS sound_chef_shared)
add_library(SoundBakery::SoundChef_Static ALIAS sound_chef_static)

target_include_directories(sound_chef_shared
    PUBLIC ${PROJECT_SOURCE_DIR}/inc/
    PRIVATE ${PROJECT_SOURCE_DIR}/src/
    PUBLIC ${stb_SOURCE_DIR}
)

target_include_directories(sound_chef_static
    PUBLIC ${PROJECT_SOURCE_DIR}/inc/
    PRIVATE ${PROJECT_SOURCE_DIR}/src/
    PUBLIC ${stb_SOURCE_DIR}
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

set(CMAKE_C_EXTENSIONS OFF)

target_include_directories(sound_chef_shared
PUBLIC ${miniaudio_SOURCE_DIR}
PRIVATE ${opusfile_SOURCE_DIR}/include
)

target_include_directories(sound_chef_static
PUBLIC ${miniaudio_SOURCE_DIR}
PRIVATE ${opusfile_SOURCE_DIR}/include
)

if (WIN32)
  message(STATUS "Fetching dirent")
  FetchContent_MakeAvailable(dirent)

  target_include_directories(sound_chef_shared
    PRIVATE ${dirent_SOURCE_DIR}/include
  )

  target_include_directories(sound_chef_static
    PRIVATE ${dirent_SOURCE_DIR}/include
  )
endif()

target_link_libraries(sound_chef_shared
    PUBLIC SoundBakery::BuildConfig
    PUBLIC clap
    PRIVATE vorbis
    PRIVATE vorbisfile
    PRIVATE vorbisenc
    PRIVATE Opus::opus
    PRIVATE OpusFile::opusfile)

target_link_libraries(sound_chef_static
    PUBLIC SoundBakery::BuildConfig
    PUBLIC clap
    PRIVATE vorbis
    PRIVATE vorbisfile
    PRIVATE vorbisenc
    PRIVATE Opus::opus
    PRIVATE OpusFile::opusfile)

get_target_property(SOUND_CHEF_ALL_FILES sound_chef_shared SOURCES)
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
  set_target_properties(sound_chef_shared PROPERTIES
    VS_GLOBAL_RunCodeAnalysis true

    # Use visual studio core guidelines
    VS_GLOBAL_EnableMicrosoftCodeAnalysis false

    # Use clangtidy
    VS_GLOBAL_EnableClangTidyCodeAnalysis true
  )
endif()

if (SOUND_BAKERY_INSTALL)
sbk_install(TARGETS sound_chef_shared DESTINATION lib/${SBK_INSTALL_CONFIG_SUBDIR}/sound_chef)
sbk_install(TARGETS sound_chef_static DESTINATION lib/${SBK_INSTALL_CONFIG_SUBDIR}/sound_chef)

sbk_install(FILES ${SOUND_CHEF_HEADERS} DESTINATION inc/sound_chef)
sbk_install(FILES ${miniaudio_SOURCE_DIR}/miniaudio.h DESTINATION inc/sound_chef)
sbk_install(FILES ${stb_SOURCE_DIR}/stb_ds.h DESTINATION inc/sound_chef)
sbk_install(DIRECTORY ${clap_SOURCE_DIR}/include/clap DESTINATION inc/sound_chef)
endif()