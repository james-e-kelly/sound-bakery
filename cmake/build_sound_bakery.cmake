macro(set_sources)
    set(SOUND_BAKERY_SOURCES 
    system.cpp
    pch.cpp

    api/sound_bakery.cpp

    error/error.cpp

    core/object/object.cpp
    core/object/object_tracker.cpp
    core/object/object_owner.cpp

    core/database/database.cpp
    core/database/database_object.cpp
    core/database/database_ptr.cpp

    core/memory.cpp
    core/name.cpp
    core/property.cpp
    core/thread_domain.cpp

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

    reflection/reflection.cpp

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

    error/error.h
    error/result.h

    core/core_include.h
    core/core_fwd.h
    core/property.h
    core/thread_domain.h
    core/name.h

    core/object/object.h
    core/object/object.inl
    core/object/object_tracker.h
    core/object/object_owner.h
    core/object/object_owner.inl

    core/database/database.h
    core/database/database_object.h
    core/database/database_ptr.h

    core/memory.h

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

    reflection/reflection.h

    serialization/serializer.h

    soundbank/soundbank.h

    sound/sound.h

    task/executor.h
    task/command_queue.h
    task/manual_executor.h
    task/thread_executor.h
    task/task.h

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

    message(STATUS "Fetching rttr...")
    FetchContent_MakeAvailable(rttr)

    message(STATUS "Fetching concurrencpp...")
    FetchContent_MakeAvailable(concurrencpp)

    # Make fmt available before spdlog and point spdlog at it, so the whole build
    # links a single fmt (fmt::fmt) instead of spdlog also compiling its bundled
    # copy. spdlog's CMake uses fmt::fmt directly when the target already exists.
    message(STATUS "Fetching fmt...")
    FetchContent_MakeAvailable(fmt)

    set(SPDLOG_FMT_EXTERNAL ON CACHE BOOL "Use external fmt instead of spdlog's bundled copy" FORCE)

    message(STATUS "Fetching spdlog...")
    FetchContent_MakeAvailable(spdlog)

    message(STATUS "Fetching boost")
    FetchContent_MakeAvailable(boost)

    message(STATUS "Fetching boost-yaml")
    FetchContent_MakeAvailable(boost-yaml)

    # Tracy (v0.13.1) has no option to disable its install() rules: it always
    # installs TracyClient.lib, its headers and a CMake package config into the
    # prefix, using its own flat/per-config scheme. We statically link
    # TracyClient into our own libraries, so consumers never need it standalone,
    # and letting it install would pollute the dist root and - because we ship
    # multiple configurations into one tree - clobber a config-specific lib at a
    # shared path.
    #
    # Override install() exactly once with a wrapper that forwards to the real
    # command (CMake exposes the overridden builtin as _install) unless a global
    # flag is set. We raise the flag only across Tracy's add_subdirectory, so
    # just its install rules are dropped and every other target still installs.
    # (Overriding install() a second time to "restore" it would rebind _install
    # to the wrapper, silently dropping all subsequent install rules.)
    set_property(GLOBAL PROPERTY SBK_SUPPRESS_INSTALL ON)
    function(install)
        get_property(_sbk_suppress_install GLOBAL PROPERTY SBK_SUPPRESS_INSTALL)
        if(NOT _sbk_suppress_install)
            _install(${ARGN})
        endif()
    endfunction()

    CPMAddPackage(
    NAME tracy
    GITHUB_REPOSITORY wolfpld/tracy
    GIT_TAG v0.13.1
    OPTIONS "TRACY_ON_DEMAND ON" "TRACY_NO_VSYNC_CAPTURE ON" "TRACY_NO_FRAME_IMAGE ON" "TRACY_FIBERS ON"
    )

    set_property(GLOBAL PROPERTY SBK_SUPPRESS_INSTALL OFF)

    message(STATUS "Fetching rpmalloc")
    FetchContent_MakeAvailable(sbk_rpmalloc_content)

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

    if (SOUND_BAKERY_FORMAT_SOURCE AND SOUND_BAKERY_CLANG_FORMAT_EXE)
        add_custom_target(format_sound_bakery
        COMMAND clang-format --style=file -i ${SOUND_BAKERY_FORMAT_FILES}
        COMMAND_EXPAND_LISTS
        COMMENT "Running clang-format"
        VERBATIM
        )
    endif()

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