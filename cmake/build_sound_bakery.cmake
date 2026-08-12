include_guard(GLOBAL)

macro(set_sources)
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

    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Source Files" FILES ${SOUND_BAKERY_SOURCES})

set(SOUND_BAKERY_HEADERS
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
    
    core/containers/ring_buffer.h
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

    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Header Files" FILES ${SOUND_BAKERY_HEADERS})

    set(PCH_HEADER pch.h)
    set(PCH_SOURCE pch.cpp)
endmacro()


function(build_dependencies)
    set(CMAKE_FOLDER extern)

    # These are global CACHE variables, forced here so the third-party dependencies fetched below
    # (and by any FetchContent-based project added afterwards, e.g. gluten's glfw/imgui/etc.) build
    # as static libraries. This relies on src/CMakeLists.txt's add_subdirectory() ordering
    # (core, sound_chef, sound_bakery, gluten): anything already configured before sound_bakery's
    # add_subdirectory() call is unaffected, but everything fetched afterwards will pick up these
    # values unless it overrides them itself.
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
        GITHUB_REPOSITORY KarateKidzz/rttr
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
    # for test-only sub-packages we don't build. We disable submodule recursion via
    # GIT_SUBMODULES "" - but CPM (0.40.x-0.43.x) does not forward that argument to
    # FetchContent, so we drop these two back to FetchContent_Declare directly.
    FetchContent_Declare(
        eabase
        GIT_REPOSITORY https://github.com/electronicarts/EABase.git
        GIT_TAG        0699a15efdfd20b6cecf02153bfa5663decb653c # matches EASTL 3.27.01's pinned EABase
        GIT_SUBMODULES ""
        GIT_PROGRESS   TRUE
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(eabase)

    set(EASTL_BUILD_BENCHMARK OFF CACHE BOOL "" FORCE)
    set(EASTL_BUILD_TESTS OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        eastl
        GIT_REPOSITORY https://github.com/electronicarts/EASTL.git
        GIT_TAG        3.27.01
        GIT_SHALLOW    TRUE
        GIT_SUBMODULES ""
        GIT_PROGRESS   TRUE
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(eastl)

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
endfunction()

macro(setup_format_sources)
    get_target_property(SOUND_BAKERY_ALL_FILES sound_bakery_shared SOURCES)
    foreach(source IN LISTS SOUND_BAKERY_ALL_FILES)
    list(APPEND SOUND_BAKERY_FORMAT_FILES "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
    endforeach()

    foreach(source IN LISTS SOUND_BAKERY_SOURCES)
    list(APPEND SOUND_BAKERY_TIDY_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
    endforeach()
endmacro()

function(setup_linter_targets)
    set(CMAKE_FOLDER src)

    sbk_add_format_target(NAME sound_bakery SOURCES ${SOUND_BAKERY_FORMAT_FILES})

    if (SOUND_BAKERY_TIDY_SOURCE AND SOUND_BAKERY_CLANG_TIDY_EXE)
        add_custom_target(tidy_sound_bakery
        COMMAND clang-tidy -fix --fix-errors --format-style=file --fix-notes -p ${CMAKE_BINARY_DIR} ${SOUND_BAKERY_TIDY_SOURCES}
        COMMAND_EXPAND_LISTS
        COMMENT "Running clang-tidy -fix"
        VERBATIM
    )
    endif()
endfunction()

function(fix_msvc_linters)
    # Sad things to make clang-tidy work in VS
    # CMakeSettings.json is possible but seemed a pain in its own right
    # https://discourse.cmake.org/t/cmake-cxx-clang-tidy-in-msvc/890/9
    if(SOUND_BAKERY_TIDY_SOURCE AND SOUND_BAKERY_CLANG_TIDY_EXE AND MSVC)
        set_target_properties(sound_bakery_shared PROPERTIES
        VS_GLOBAL_RunCodeAnalysis false
        # Use visual studio core guidelines
        VS_GLOBAL_EnableMicrosoftCodeAnalysis false
        # Use clangtidy
        VS_GLOBAL_EnableClangTidyCodeAnalysis true
        )
        set_target_properties(sound_bakery_static PROPERTIES
        VS_GLOBAL_RunCodeAnalysis false
        # Use visual studio core guidelines
        VS_GLOBAL_EnableMicrosoftCodeAnalysis false
        # Use clangtidy
        VS_GLOBAL_EnableClangTidyCodeAnalysis true
        )
    endif()
endfunction()

macro(set_big_objects)
    if(MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
    endif()
endmacro()