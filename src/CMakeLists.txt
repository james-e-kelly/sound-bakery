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

c_17(delegates)
cxx_20(delegates)

add_library(SoundBakery::Delegates ALIAS delegates)

set(CMAKE_FOLDER src)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:__cplusplus")
endif()

add_subdirectory(sound_chef)
add_subdirectory(sound_bakery)
add_subdirectory(gluten)