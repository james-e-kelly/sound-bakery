message(STATUS "Adding Sound Bakery")

set(ATLAS_SOURCES 
    system.cpp
    factory.cpp
    pch.cpp

    core/object/object.cpp
    core/object/object_tracker.cpp

    core/database/database.cpp
    core/database/database_object.cpp
    core/database/database_ptr.cpp

    core/property.cpp
    
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
    
    serialization/serializer.cpp
    
    sound/sound.cpp

    util/type_helper.cpp

    voice/voice.cpp
    voice/node_instance.cpp
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Source Files" FILES ${ATLAS_SOURCES})

set(ATLAS_HEADERS
    sound_bakery.h
    system.h
    factory.h
    pch.h

    core/core_include.h
    core/core_fwd.h
    core/property.h
    
    core/object/object.h
    core/object/object_global.h
    core/object/object_tracker.h
    
    core/database/database.h
    core/database/database_object.h
    core/database/database_ptr.h
    
    effect/effect.h
    
    event/event.h
    
    editor/editor_defines.h
    
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
    
    serialization/serializer.h
    
    sound/sound.h
    
    util/api_macros.h
    util/fmod_pointers.h
    util/sb_pointers.h
    util/singleton.h
    util/type_helper.h
    util/macros.h

    voice/voice.h
    voice/node_instance.h
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Header Files" FILES ${ATLAS_HEADERS})

set(PCH_HEADER pch.h)
set(PCH_SOURCE pch.cpp)

add_library(sound_bakery STATIC ${ATLAS_SOURCES} ${ATLAS_HEADERS} ${PCH_SOURCE})
add_library(SoundBakery::SoundBakery ALIAS sound_bakery)
target_include_directories(sound_bakery
    PUBLIC ${CMAKE_SOURCE_DIR}/src/
)

target_precompile_headers(sound_bakery PRIVATE ${PCH_HEADER})

set(BUILD_STATIC ON CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_WITH_STATIC_RUNTIME_LIBS ON CACHE BOOL "" FORCE)
set(BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_DOCUMENTATION OFF CACHE BOOL "" FORCE)
set(BUILD_INSTALLER OFF CACHE BOOL "" FORCE)
set(BUILD_PACKAGE OFF CACHE BOOL "" FORCE)
set(CUSTOM_DOXYGEN_STYLE OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  rttr
  GIT_REPOSITORY https://github.com/rttrorg/rttr.git
  GIT_TAG        7edbd580cfad509a3253c733e70144e36f02ecd4
  GIT_SHALLOW    TRUE
  GIT_PROGRESS TRUE
  OVERRIDE_FIND_PACKAGE
)

FetchContent_Declare(
  yaml
  GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
  GIT_TAG        0.8.0
  GIT_SHALLOW    TRUE
  GIT_PROGRESS TRUE
  OVERRIDE_FIND_PACKAGE
)

message(STATUS "Fetching rttr...")
FetchContent_MakeAvailable(rttr)

FetchContent_GetProperties(yaml)

if(NOT yaml_POPULATED)
  message(STATUS "Fetching yaml-cpp...")
  set(YAML_CPP_FORMAT_SOURCE OFF CACHE BOOL "" FORCE)
  FetchContent_Populate(yaml)
  add_subdirectory(${yaml_SOURCE_DIR} ${yaml_BINARY_DIR})
endif()

target_link_libraries(sound_bakery PUBLIC 
  SoundBakery::Delegates 
  SoundBakery::SoundChef
  RTTR::Core_Lib
  yaml-cpp::yaml-cpp)

if(MSVC)
  target_compile_options(sound_bakery PRIVATE /W4)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
else()
  target_compile_options(sound_bakery PRIVATE -Wall -Wextra -Wpedantic)
endif()

if (SOUND_BAKERY_FORMAT_SOURCE AND SOUND_BAKERY_CLANG_FORMAT_EXE)
  get_target_property(SOUND_BAKERY_ALL_FILES sound_bakery SOURCES)

  foreach(source IN LISTS SOUND_BAKERY_ALL_FILES)
    list(APPEND SOUND_BAKERY_FORMAT_FILES "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
  endforeach()

  add_custom_target(format_sound_bakery
  COMMAND clang-format --style=file -i ${SOUND_BAKERY_FORMAT_FILES}
  COMMAND_EXPAND_LISTS
  COMMENT "Running clang-format"
  VERBATIM
  )
endif()