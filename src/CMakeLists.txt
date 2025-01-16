# clang-tidy
option(SOUND_BAKERY_TIDY "Run clang-tidy" ON)

set(CMAKE_FOLDER extern)

message(STATUS "Fetching CppDelegates")
FetchContent_MakeAvailable(cppdelegates)

message(STATUS "Fetching stb")
FetchContent_MakeAvailable(stb)

add_library(delegates STATIC 
    ${cppdelegates_SOURCE_DIR}/Delegates.cpp
)

target_include_directories(delegates
    PUBLIC ${cppdelegates_SOURCE_DIR}
)

set_property(TARGET delegates PROPERTY CXX_STANDARD 20)

add_library(SoundBakery::Delegates ALIAS delegates)

set(CMAKE_FOLDER src)

add_subdirectory(sound_chef)
add_subdirectory(sound_bakery)
add_subdirectory(gluten)