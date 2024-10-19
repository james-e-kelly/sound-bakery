macro(set_sources)
    set(SOUND_BAKERY_SOURCES 
    system.cpp
    factory.cpp
    pch.cpp

    core/object/object.cpp
    core/object/object_tracker.cpp
    core/object/object_owner.cpp

    core/database/database.cpp
    core/database/database_object.cpp
    core/database/database_ptr.cpp

    core/property.cpp

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
    voice/node_instance_impl.cpp
    )

    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Source Files" FILES ${SOUND_BAKERY_SOURCES})

    set(SOUND_BAKERY_HEADERS
    sound_bakery_internal.h
    system.h
    factory.h
    pch.h

    core/core_include.h
    core/core_fwd.h
    core/property.h

    core/object/object.h
    core/object/object_tracker.h
    core/object/object_owner.h

    core/database/database.h
    core/database/database_object.h
    core/database/database_ptr.h

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

    util/api_macros.h
    util/fmod_pointers.h
    util/sb_pointers.h
    util/type_helper.h
    util/macros.h

    voice/voice.h
    voice/node_instance.h
    )

    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Header Files" FILES ${SOUND_BAKERY_HEADERS})

    set(PCH_HEADER pch.h)
    set(PCH_SOURCE pch.cpp)
endmacro()


function(build_dependencies)
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

    message(STATUS "Fetching rttr...")
    FetchContent_MakeAvailable(rttr)

    message(STATUS "Fetching concurrencpp...")
    FetchContent_MakeAvailable(concurrencpp)

    message(STATUS "Fetching spdlog...")
    FetchContent_MakeAvailable(spdlog)

    message(STATUS "Fetching yaml-cpp...")
    set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
    set(YAML_CPP_FORMAT_SOURCE OFF CACHE BOOL "" FORCE)
    set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
    set(YAML_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(yaml)
endfunction()

macro(setup_format_sources)
    get_target_property(SOUND_BAKERY_ALL_FILES sound_bakery SOURCES)
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
        set_target_properties(sound_bakery PROPERTIES
        VS_GLOBAL_RunCodeAnalysis false

        # Use visual studio core guidelines
        VS_GLOBAL_EnableMicrosoftCodeAnalysis false

        # Use clangtidy
        VS_GLOBAL_EnableClangTidyCodeAnalysis true
    )
    endif()
endfunction()

function(set_big_objects)
    if(MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
    endif()
endfunction()