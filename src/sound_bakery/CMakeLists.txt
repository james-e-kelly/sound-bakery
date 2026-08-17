message(STATUS "Adding Sound Bakery")

# -----------------------------------------------------------------------------
# Version header (generated) - written to build tree, included from source, copied on install
# -----------------------------------------------------------------------------
set(SOUND_BAKERY_GENERATED_INC_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated/inc)

set(SOUND_BAKERY_GENERATED_HEADERS
    ${SOUND_BAKERY_GENERATED_INC_DIR}/sound_bakery/sound_bakery_version.h
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/sound_bakery_version.h.in
    ${SOUND_BAKERY_GENERATED_INC_DIR}/sound_bakery/sound_bakery_version.h
    @ONLY
)

# -----------------------------------------------------------------------------
# Dependencies
#
# These CACHE vars are FORCE'd so third-party dependencies fetched below (and
# any FetchContent-based project added afterwards, e.g. gluten's glfw/imgui)
# build as static libraries. Relies on src/CMakeLists.txt's add_subdirectory()
# ordering (core, sound_chef, sound_bakery, gluten): anything already
# configured before sound_bakery is unaffected, but everything fetched
# afterwards picks up these values unless it overrides them itself.
# -----------------------------------------------------------------------------
set(CMAKE_FOLDER extern)

set(BUILD_STATIC ON CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_WITH_STATIC_RUNTIME_LIBS ON CACHE BOOL "" FORCE)
set(BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_DOCUMENTATION OFF CACHE BOOL "" FORCE)
set(BUILD_INSTALLER OFF CACHE BOOL "" FORCE)
set(BUILD_PACKAGE OFF CACHE BOOL "" FORCE)
set(CUSTOM_DOXYGEN_STYLE OFF CACHE BOOL "" FORCE)

CPMAddPackage(
    NAME rttr
    GITHUB_REPOSITORY james-e-kelly/rttr
    GIT_TAG 23850d1dd23952b7c29ba9398adb1548f6816c30 # master @ 2026-07-17
    EXCLUDE_FROM_ALL YES
)

CPMAddPackage(
    NAME concurrencpp
    GITHUB_REPOSITORY james-e-kelly/concurrencpp
    GIT_TAG 53b007ba3721d99ec3aa6c39dd228638798d54e4 # master @ 2026-07-17
    EXCLUDE_FROM_ALL YES
)

# EABase and EASTL declare their .gitmodules using git@github.com: (SSH) URLs
# for test-only sub-packages we don't build. The usual way to skip submodule
# recursion is GIT_SUBMODULES "", but CPM's arg parser drops empty-string
# values before forwarding to FetchContent (cpm-cmake/CPM.cmake#467), so we
# use GIT_CONFIG submodule.active=none instead - a non-empty arg CPM forwards
# cleanly that tells git to skip every submodule.
CPMAddPackage(
    NAME eabase
    GITHUB_REPOSITORY electronicarts/EABase
    GIT_TAG 0699a15efdfd20b6cecf02153bfa5663decb653c # matches EASTL 3.27.01's pinned EABase
    GIT_CONFIG submodule.active=none
    EXCLUDE_FROM_ALL YES
)

set(EASTL_BUILD_BENCHMARK OFF CACHE BOOL "" FORCE)
set(EASTL_BUILD_TESTS OFF CACHE BOOL "" FORCE)

CPMAddPackage(
    NAME eastl
    GITHUB_REPOSITORY electronicarts/EASTL
    GIT_TAG 3.27.01
    GIT_SHALLOW TRUE
    GIT_CONFIG submodule.active=none
    EXCLUDE_FROM_ALL YES
)

# Make fmt available before spdlog and point spdlog at it, so the whole build
# links a single fmt (fmt::fmt) instead of spdlog also compiling its bundled
# copy. spdlog's CMake uses fmt::fmt directly when the target already exists.
CPMAddPackage(
    NAME fmt
    GITHUB_REPOSITORY fmtlib/fmt
    GIT_TAG 12.2.0
    EXCLUDE_FROM_ALL YES
)

set(SPDLOG_FMT_EXTERNAL ON CACHE BOOL "Use external fmt instead of spdlog's bundled copy" FORCE)

CPMAddPackage(
    NAME spdlog
    GITHUB_REPOSITORY gabime/spdlog
    GIT_TAG v1.17.0
    EXCLUDE_FROM_ALL YES
)

CPMAddPackage(
    NAME boost
    URL https://github.com/boostorg/boost/releases/download/boost-1.87.0/boost-1.87.0-cmake.tar.xz
    EXCLUDE_FROM_ALL YES
)

CPMAddPackage(
    NAME boost-yaml
    GITHUB_REPOSITORY james-e-kelly/yaml-archive
    GIT_TAG fdbba62f97d16a0e8c28dcf04724423293752bc5 # HEAD @ 2026-07-17
    EXCLUDE_FROM_ALL YES
)

# Tracy (v0.13.1) has no option to disable its install() rules (TracyClient.lib,
# its headers and CMake package config); they are suppressed by default like
# every dependency (see setup_install_suppression.cmake). We statically link
# TracyClient into our own libraries, so consumers never need it standalone.
CPMAddPackage(
    NAME tracy
    GITHUB_REPOSITORY wolfpld/tracy
    GIT_TAG v0.13.1
    OPTIONS "TRACY_ON_DEMAND ON" "TRACY_NO_VSYNC_CAPTURE ON" "TRACY_NO_FRAME_IMAGE ON" "TRACY_FIBERS ON"
)

CPMAddPackage(
    NAME sbk_rpmalloc_content
    GITHUB_REPOSITORY mjansson/rpmalloc
    GIT_TAG 1.4.5
    DOWNLOAD_ONLY YES
)

add_library(sbk_rpmalloc STATIC ${sbk_rpmalloc_content_SOURCE_DIR}/rpmalloc/rpmalloc.c)
target_include_directories(sbk_rpmalloc PUBLIC ${sbk_rpmalloc_content_SOURCE_DIR})
target_compile_definitions(sbk_rpmalloc PUBLIC ENABLE_STATISTICS)
add_library(sbk::rpmalloc ALIAS sbk_rpmalloc)
set_target_properties(sbk_rpmalloc PROPERTIES C_EXTENSIONS ON)

# -----------------------------------------------------------------------------
# Source / header lists
# -----------------------------------------------------------------------------
set(SOUND_BAKERY_SOURCES
    system.cpp
    pch.cpp

    api/sound_bakery.cpp

    core/name.cpp
    core/property.cpp
    core/thread_domain.cpp

    core/error/error.cpp

    core/database/database.cpp
    core/database/database_object.cpp
    core/database/database_ptr.cpp

    core/memory/memory.cpp

    core/object/object.cpp
    core/object/object_tracker.cpp
    core/object/object_owner.cpp

    core/reflection/reflection.cpp

    core/task/command_queue.cpp
    core/task/system_thread.cpp

    editor/project/project.cpp
    editor/project/project_configuration.cpp

    effect/effect.cpp

    event/event.cpp

    gameobject/gameobject.cpp

    node/node.cpp

    node/bus/bus.cpp
    node/bus/aux_bus.cpp

    node/container/container.cpp
    node/container/blend_container.cpp
    node/container/random_container.cpp
    node/container/sequence_container.cpp
    node/container/sound_container.cpp
    node/container/switch_container.cpp

    parameter/parameter.cpp
    profiling/voice_tracker.cpp

    runtime/runtime.cpp

    serialization/serializer.cpp

    soundbank/soundbank.cpp

    sound/sound.cpp

    util/type_helper.cpp

    voice/voice.cpp
    voice/node_instance.cpp
)

set(SOUND_BAKERY_PRIVATE_HEADERS
    sound_bakery_internal.h
    system.h
    pch.h

    api/engine_api.h

    core/core_fwd.h
    core/property.h
    core/thread_domain.h
    core/name.h

    core/error/error.h
    core/error/result.h

    core/containers/message_queue.h

    core/database/database.h
    core/database/database_object.h
    core/database/database_ptr.h

    core/memory/allocator.h
    core/memory/eastl_config.h
    core/memory/memory.h

    core/object/object.h
    core/object/object_tracker.h
    core/object/object_owner.h

    core/reflection/reflection.h
    core/reflection/eastl_reflection.h

    core/task/executor.h
    core/task/command_queue.h
    core/task/manual_executor.h
    core/task/thread_executor.h
    core/task/task.h
    core/task/unique_coroutine.h
    core/task/work_item.h
    core/task/system_thread.h

    editor/editor_defines.h
    editor/project/project.h
    editor/project/project_configuration.h

    effect/effect.h

    event/event.h

    gameobject/gameobject.h

    maths/easing.h

    node/node.h

    node/bus/bus.h
    node/bus/aux_bus.h

    node/container/container.h
    node/container/blend_container.h
    node/container/random_container.h
    node/container/sequence_container.h
    node/container/sound_container.h
    node/container/switch_container.h

    parameter/parameter.h

    profiling/voice_tracker.h

    runtime/runtime.h

    serialization/serializer.h
    serialization/eastl_serialization.h

    soundbank/soundbank.h

    sound/sound.h

    util/type_helper.h

    voice/voice.h
    voice/node_instance.h
)

set(SOUND_BAKERY_PUBLIC_HEADERS
    ${PROJECT_SOURCE_DIR}/inc/sound_bakery/sound_bakery.h
    ${PROJECT_SOURCE_DIR}/inc/sound_bakery/sound_bakery_common.h
)

# -----------------------------------------------------------------------------
# Library targets
# -----------------------------------------------------------------------------
set(CMAKE_FOLDER src)

add_library(sound_bakery_shared SHARED)
add_library(sound_bakery_static STATIC)

add_library(SoundBakery::SoundBakery_Shared ALIAS sound_bakery_shared)
add_library(SoundBakery::SoundBakery_Static ALIAS sound_bakery_static)

set_target_properties(sound_bakery_shared PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>)

foreach(t IN ITEMS sound_bakery_shared sound_bakery_static)
    c_17(${t})
    cxx_20(${t})

    target_precompile_headers(${t} PRIVATE pch.h)

    target_compile_options(${t} PRIVATE ${PROJECT_LIBRARY_WARNING_FLAGS})

    target_sources(${t}
        PRIVATE
            ${SOUND_BAKERY_SOURCES}
            ${SOUND_BAKERY_PRIVATE_HEADERS}
        PUBLIC
            FILE_SET HEADERS
            BASE_DIRS
                ${PROJECT_SOURCE_DIR}/inc
                ${SOUND_BAKERY_GENERATED_INC_DIR}
            FILES
                ${SOUND_BAKERY_PUBLIC_HEADERS}
                ${SOUND_BAKERY_GENERATED_HEADERS}
    )

    target_include_directories(${t}
        PUBLIC ${PROJECT_SOURCE_DIR}/src/
    )

    target_link_libraries(${t}
        PUBLIC  SoundBakery::BuildConfig
        PUBLIC  SoundBakery::Core
        PUBLIC  RTTR::Core_Lib
        PUBLIC  Boost::serialization
        PUBLIC  Boost::msm
        PUBLIC  yaml-archive
        PUBLIC  Tracy::TracyClient
        PUBLIC  EASTL
        PRIVATE sbk::rpmalloc
    )
endforeach()

target_link_libraries(sound_bakery_shared PUBLIC
    SoundBakery::SoundChef_Shared
    spdlog::spdlog
)

target_link_libraries(sound_bakery_static PUBLIC
    SoundBakery::SoundChef_Static
)

# -----------------------------------------------------------------------------
# IDE folder hierarchy
# -----------------------------------------------------------------------------
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}            PREFIX "Source Files"    FILES ${SOUND_BAKERY_SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}            PREFIX "Private Headers" FILES ${SOUND_BAKERY_PRIVATE_HEADERS})
source_group(TREE ${PROJECT_SOURCE_DIR}/inc/sound_bakery PREFIX "Public Headers"  FILES ${SOUND_BAKERY_PUBLIC_HEADERS})
source_group("Generated Headers" FILES ${SOUND_BAKERY_GENERATED_HEADERS})

# -----------------------------------------------------------------------------
# Install
# -----------------------------------------------------------------------------
if (SOUND_BAKERY_INSTALL)
    foreach(t IN ITEMS sound_bakery_shared sound_bakery_static)
        sbk_install(TARGETS ${t}
            RUNTIME  DESTINATION lib/${SBK_INSTALL_CONFIG_SUBDIR}/sound_bakery
            LIBRARY  DESTINATION lib/${SBK_INSTALL_CONFIG_SUBDIR}/sound_bakery
            ARCHIVE  DESTINATION lib/${SBK_INSTALL_CONFIG_SUBDIR}/sound_bakery
            FILE_SET HEADERS DESTINATION inc
        )
    endforeach()
endif()

# -----------------------------------------------------------------------------
# Format / lint
# -----------------------------------------------------------------------------
set(CMAKE_FOLDER src)

sbk_add_format_target(
    NAME sound_bakery
    SOURCES
        ${SOUND_BAKERY_SOURCES}
        ${SOUND_BAKERY_PRIVATE_HEADERS}
        ${SOUND_BAKERY_PUBLIC_HEADERS}
        ${SOUND_BAKERY_GENERATED_HEADERS}
)

# Sad things to make clang-tidy work in VS
# CMakeSettings.json is possible but seemed a pain in its own right
# https://discourse.cmake.org/t/cmake-cxx-clang-tidy-in-msvc/890/9
if(SOUND_BAKERY_TIDY_SOURCE AND SOUND_BAKERY_CLANG_TIDY_EXE AND MSVC)
    foreach(t IN ITEMS sound_bakery_shared sound_bakery_static)
        set_target_properties(${t} PROPERTIES
            VS_GLOBAL_RunCodeAnalysis false
            VS_GLOBAL_EnableMicrosoftCodeAnalysis false
            VS_GLOBAL_EnableClangTidyCodeAnalysis true
        )
    endforeach()
endif()
