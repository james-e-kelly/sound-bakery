# clang-tidy
option(SOUND_BAKERY_TIDY "Run clang-tidy" ON)

set(CMAKE_FOLDER extern)

message(STATUS "Fetching CppDelegates")

FetchContent_Declare(
  cppdelegates
  GIT_REPOSITORY https://github.com/KarateKidzz/CppDelegates.git
  GIT_TAG        master
  GIT_SHALLOW    TRUE
  GIT_PROGRESS TRUE
  OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(cppdelegates)

add_library(delegates STATIC 
    ${cppdelegates_SOURCE_DIR}/Delegates.cpp
)

target_include_directories(delegates
    PUBLIC ${cppdelegates_SOURCE_DIR}
)

set_property(TARGET delegates PROPERTY CXX_STANDARD 20)

add_library(SoundBakery::Delegates ALIAS delegates)

set(CMAKE_FOLDER src)

add_subdirectory(gluten)
add_subdirectory(sound_chef)
add_subdirectory(sound_bakery)