message(STATUS "Adding Sound Chef")

# -----------------------------------------------------------------------------
# Version header (generated) - written to build tree, included from source, copied on install
# -----------------------------------------------------------------------------
set(SOUND_CHEF_GENERATED_INC_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated/inc)

set(SOUND_CHEF_GENERATED_HEADERS
    ${SOUND_CHEF_GENERATED_INC_DIR}/sound_chef/sound_chef_version.h
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/sound_chef_version.h.in
    ${SOUND_CHEF_GENERATED_INC_DIR}/sound_chef/sound_chef_version.h
    @ONLY
)

# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------
set(CMAKE_FOLDER extern)

CPMAddPackage(
    NAME miniaudio
    GITHUB_REPOSITORY mackron/miniaudio
    GIT_TAG 0.11.21
    EXCLUDE_FROM_ALL YES
)

CPMAddPackage(
    NAME clap
    GITHUB_REPOSITORY free-audio/clap
    GIT_TAG 1.2.2
    EXCLUDE_FROM_ALL YES
)

CPMAddPackage(
    NAME c89atomic
    GITHUB_REPOSITORY mackron/c89atomic
    GIT_TAG bf5c8af9b89bf79ba1705d10c361dcb0699304a2 # master @ 2026-07-30
    DOWNLOAD_ONLY YES
)

add_library(c89atomic STATIC)
target_sources(c89atomic
    PRIVATE ${c89atomic_SOURCE_DIR}/c89atomic.c
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${c89atomic_SOURCE_DIR}
        FILES     ${c89atomic_SOURCE_DIR}/c89atomic.h
)
add_library(c89atomic::c89atomic ALIAS c89atomic)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# Xiph libraries need C_EXTENSIONS on Linux for using alloca
set(CMAKE_C_EXTENSIONS ON)

CPMAddPackage(
    NAME ogg
    GITHUB_REPOSITORY xiph/ogg
    GIT_TAG v1.3.5
    EXCLUDE_FROM_ALL YES
)

CPMAddPackage(
    NAME vorbis
    GITHUB_REPOSITORY xiph/vorbis
    GIT_TAG v1.3.7
    EXCLUDE_FROM_ALL YES
)

CPMAddPackage(
    NAME opus
    GITHUB_REPOSITORY xiph/opus
    GIT_TAG v1.5.2
    EXCLUDE_FROM_ALL YES
)

CPMAddPackage(
    NAME opusfile
    GITHUB_REPOSITORY KarateKidzz/opusfile
    GIT_TAG 8cf05127fb1ae65735c2db6f65ad89e7a32ac74f # master @ 2026-07-17
    OPTIONS
        "OP_DISABLE_EXAMPLES ON"
        "OP_DISABLE_DOCS ON"
        "OP_DISABLE_HTTP ON"
    EXCLUDE_FROM_ALL YES
)

set(CMAKE_C_EXTENSIONS OFF)

if (WIN32)
    CPMAddPackage(
        NAME dirent
        GITHUB_REPOSITORY tronkko/dirent
        GIT_TAG 31db6474b5231c180bd4618c8c90e43af50c86d0 # HEAD @ 2026-07-17
        DOWNLOAD_ONLY YES
    )
endif()

# -----------------------------------------------------------------------------
# Source / header lists
# -----------------------------------------------------------------------------
set(SOUND_CHEF_SOURCES
    sound_chef.c
    sound_chef_bank.c
    sound_chef_encoder.c
    sound_chef_sound.c
    sound_chef_node_group.c

    dsp/sound_chef_dsp.c
    dsp/sound_chef_dsp_clap.c
    dsp/sound_chef_dsp_delay.c
    dsp/sound_chef_dsp_fader.c
    dsp/sound_chef_dsp_highpass.c
    dsp/sound_chef_dsp_lowpass.c
    dsp/sound_chef_dsp_meter.c

    encoders/sound_chef_encoder_vorbis.c
    encoders/vorbis_encode_test.c

    decoders/sound_chef_decoder_opus.c
    decoders/sound_chef_decoder_vorbis.c
)

set(SOUND_CHEF_PRIVATE_HEADERS
    sound_chef_guide.h
)

set(SOUND_CHEF_PUBLIC_HEADERS
    ${PROJECT_SOURCE_DIR}/inc/sound_chef/sound_chef.h
)

# -----------------------------------------------------------------------------
# Library targets
# -----------------------------------------------------------------------------
set(CMAKE_FOLDER src)

add_library(sound_chef_shared SHARED)
add_library(sound_chef_static STATIC)

add_library(SoundBakery::SoundChef_Shared ALIAS sound_chef_shared)
add_library(SoundBakery::SoundChef_Static ALIAS sound_chef_static)

set_target_properties(sound_chef_shared PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>)

foreach(t IN ITEMS sound_chef_shared sound_chef_static)
    c_17(${t})
    cxx_20(${t})

    target_compile_options(${t} PRIVATE ${PROJECT_LIBRARY_WARNING_FLAGS})

    target_sources(${t}
        PRIVATE
            ${SOUND_CHEF_SOURCES}
            ${SOUND_CHEF_PRIVATE_HEADERS}
        PUBLIC
            FILE_SET HEADERS
            BASE_DIRS
                ${PROJECT_SOURCE_DIR}/inc
                ${SOUND_CHEF_GENERATED_INC_DIR}
            FILES
                ${SOUND_CHEF_PUBLIC_HEADERS}
                ${SOUND_CHEF_GENERATED_HEADERS}
    )

    target_include_directories(${t}
        PRIVATE ${PROJECT_SOURCE_DIR}/src/
        PUBLIC  ${miniaudio_SOURCE_DIR}
        PRIVATE ${opusfile_SOURCE_DIR}/include
    )

    if (WIN32)
        target_include_directories(${t} PRIVATE ${dirent_SOURCE_DIR}/include)
    endif()

    target_link_libraries(${t}
        PUBLIC  SoundBakery::BuildConfig
        PUBLIC  clap
        PUBLIC  c89atomic::c89atomic
        PRIVATE vorbis
        PRIVATE vorbisfile
        PRIVATE vorbisenc
        PRIVATE Opus::opus
        PRIVATE OpusFile::opusfile
    )
endforeach()

# -----------------------------------------------------------------------------
# IDE folder hierarchy
# -----------------------------------------------------------------------------
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}          PREFIX "Source Files"    FILES ${SOUND_CHEF_SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}          PREFIX "Private Headers" FILES ${SOUND_CHEF_PRIVATE_HEADERS})
source_group(TREE ${PROJECT_SOURCE_DIR}/inc/sound_chef PREFIX "Public Headers"  FILES ${SOUND_CHEF_PUBLIC_HEADERS})
source_group("Generated Headers" FILES ${SOUND_CHEF_GENERATED_HEADERS})

# -----------------------------------------------------------------------------
# Install
# -----------------------------------------------------------------------------
if (SOUND_BAKERY_INSTALL)
    foreach(t IN ITEMS sound_chef_shared sound_chef_static)
        sbk_install(TARGETS ${t}
            RUNTIME  DESTINATION lib/${SBK_INSTALL_CONFIG_SUBDIR}/sound_chef
            LIBRARY  DESTINATION lib/${SBK_INSTALL_CONFIG_SUBDIR}/sound_chef
            ARCHIVE  DESTINATION lib/${SBK_INSTALL_CONFIG_SUBDIR}/sound_chef
            FILE_SET HEADERS DESTINATION inc
        )
    endforeach()

    # Third-party public headers we re-expose to Sound Chef consumers.
    sbk_install(FILES ${miniaudio_SOURCE_DIR}/miniaudio.h DESTINATION inc/sound_chef)
    sbk_install(FILES ${c89atomic_SOURCE_DIR}/c89atomic.h DESTINATION inc/sound_chef)
    sbk_install(DIRECTORY ${clap_SOURCE_DIR}/include/clap DESTINATION inc/sound_chef)
endif()

# -----------------------------------------------------------------------------
# Format / lint
# -----------------------------------------------------------------------------
set(CMAKE_FOLDER src)

sbk_add_format_target(
    NAME sound_chef
    SOURCES
        ${SOUND_CHEF_SOURCES}
        ${SOUND_CHEF_PRIVATE_HEADERS}
        ${SOUND_CHEF_PUBLIC_HEADERS}
        ${SOUND_CHEF_GENERATED_HEADERS}
)

# Sad things to make clang-tidy work in VS
# CMakeSettings.json is possible but seemed a pain in its own right
# https://discourse.cmake.org/t/cmake-cxx-clang-tidy-in-msvc/890/9
if(SOUND_BAKERY_TIDY_SOURCE AND SOUND_BAKERY_CLANG_TIDY_EXE AND MSVC)
    set_target_properties(sound_chef_shared PROPERTIES
        VS_GLOBAL_RunCodeAnalysis true
        VS_GLOBAL_EnableMicrosoftCodeAnalysis false
        VS_GLOBAL_EnableClangTidyCodeAnalysis true
    )
endif()
