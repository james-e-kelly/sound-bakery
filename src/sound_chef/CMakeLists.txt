message(STATUS "Adding Sound Chef")

# -----------------------------------------------------------------------------
# Version header (generated)
# -----------------------------------------------------------------------------
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/sound_chef_version.h.in
    ${PROJECT_SOURCE_DIR}/inc/sound_chef/sound_chef_version.h
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
# Core object library - consumed by both static and shared libraries
# -----------------------------------------------------------------------------
set(CMAKE_FOLDER src)

add_library(sound_chef_objects OBJECT)

target_sources(sound_chef_objects
    PRIVATE
        sound_chef.c
        sound_chef_bank.c
        sound_chef_encoder.c
        sound_chef_internal.c

        dsp/sound_chef_dsp.c
        dsp/sound_chef_dsp_clap.c
        dsp/sound_chef_dsp_delay.c
        dsp/sound_chef_dsp_fader.c
        dsp/sound_chef_dsp_highpass.c
        dsp/sound_chef_dsp_lowpass.c
        dsp/sound_chef_dsp_meter.c

        encoders/sound_chef_encoder_vorbis.c
        encoders/vorbis_encode_test.c
    PRIVATE
        FILE_SET sound_chef_local_headers TYPE HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES
            sound_chef_bank.h
            sound_chef_encoder.h
            sound_chef_guide.h
            sound_chef_internal.h

            encoders/sound_chef_encoder_vorbis.h
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${PROJECT_SOURCE_DIR}/inc
        FILES
            ${PROJECT_SOURCE_DIR}/inc/sound_chef/sound_chef.h
            ${PROJECT_SOURCE_DIR}/inc/sound_chef/sound_chef_common.h
            ${PROJECT_SOURCE_DIR}/inc/sound_chef/sound_chef_dsp.h
            ${PROJECT_SOURCE_DIR}/inc/sound_chef/sound_chef_version.h
)

c_17(sound_chef_objects)
cxx_20(sound_chef_objects)

# PIC on the object library lets the same objects feed both the SHARED and the
# STATIC wrapper without a second compile.
set_target_properties(sound_chef_objects PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_compile_options(sound_chef_objects PRIVATE ${PROJECT_LIBRARY_WARNING_FLAGS})

target_include_directories(sound_chef_objects
    PRIVATE ${PROJECT_SOURCE_DIR}/src/
    PUBLIC  ${miniaudio_SOURCE_DIR}
    PRIVATE ${opusfile_SOURCE_DIR}/include
)

if (WIN32)
    target_include_directories(sound_chef_objects PRIVATE ${dirent_SOURCE_DIR}/include)
endif()

target_link_libraries(sound_chef_objects
    PUBLIC  SoundBakery::BuildConfig
    PUBLIC  clap
    PUBLIC  c89atomic::c89atomic
    PRIVATE vorbis
    PRIVATE vorbisfile
    PRIVATE vorbisenc
    PRIVATE Opus::opus
    PRIVATE OpusFile::opusfile
)

# -----------------------------------------------------------------------------
# Static and shared libraries
# -----------------------------------------------------------------------------
add_library(sound_chef_shared SHARED)
add_library(sound_chef_static STATIC)

target_link_libraries(sound_chef_shared PUBLIC sound_chef_objects)
target_link_libraries(sound_chef_static PUBLIC sound_chef_objects)

add_library(SoundBakery::SoundChef_Shared ALIAS sound_chef_shared)
add_library(SoundBakery::SoundChef_Static ALIAS sound_chef_static)

set_target_properties(sound_chef_shared PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>)

# -----------------------------------------------------------------------------
# Install
# -----------------------------------------------------------------------------
if (SOUND_BAKERY_INSTALL)
    sbk_install(TARGETS sound_chef_objects
        FILE_SET HEADERS DESTINATION inc
    )

    foreach(t IN ITEMS sound_chef_shared sound_chef_static)
        sbk_install(TARGETS ${t}
            RUNTIME DESTINATION lib/${SBK_INSTALL_CONFIG_SUBDIR}/sound_chef
            LIBRARY DESTINATION lib/${SBK_INSTALL_CONFIG_SUBDIR}/sound_chef
            ARCHIVE DESTINATION lib/${SBK_INSTALL_CONFIG_SUBDIR}/sound_chef
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

get_target_property(_sound_chef_format_files sound_chef_objects SOURCES)

sbk_add_format_target(
    NAME sound_chef
    SOURCES ${_sound_chef_format_files}
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
