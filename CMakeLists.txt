cmake_minimum_required(VERSION 3.28) # Version set to have EXCLUDE_FROM_ALL available in FetchContent_Declare

project(
  "Sound Bakery" 
  VERSION 0.3.0 
  DESCRIPTION "An open-source audio middleware tool with an authoring application and programmer's API"
  HOMEPAGE_URL "https://soundbakery.jameskelly.audio"
)

include(${CMAKE_SOURCE_DIR}/cmake/setup_init.cmake)

option(SOUND_BAKERY_EDITOR "Build the Sound Bakery editor" ${SOUND_BAKERY_MAIN_PROJECT})
option(SOUND_BAKERY_SHARED "Build Sound Bakery and Sound Chef as shared libraries" OFF)
option(SOUND_BAKERY_DOCS "Build doxygen docs" OFF)
option(SOUND_BAKERY_TESTS "Build test library" OFF)
option(SOUND_BAKERY_FORMAT_SOURCE "Format source" ${SOUND_BAKERY_MAIN_PROJECT})
option(SOUND_BAKERY_TIDY_SOURCE "Run clang-tidy on source files" OFF)

message(STATUS "C Compiler - ${CMAKE_C_COMPILER} ${CMAKE_C_COMPILER_VERSION}")
message(STATUS "CXX Compiler - ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "SOUND_BAKERY_MAIN_PROJECT=${SOUND_BAKERY_MAIN_PROJECT}")
message(STATUS "SOUND_BAKERY_EDITOR=${SOUND_BAKERY_EDITOR}")
message(STATUS "SOUND_BAKERY_SHARED=${SOUND_BAKERY_SHARED}")
message(STATUS "SOUND_BAKERY_EDITOR=${SOUND_BAKERY_EDITOR}")
message(STATUS "SOUND_BAKERY_SHARED=${SOUND_BAKERY_SHARED}")
message(STATUS "SOUND_BAKERY_DOCS=${SOUND_BAKERY_DOCS}")
message(STATUS "SOUND_BAKERY_TESTS=${SOUND_BAKERY_TESTS}")
message(STATUS "SOUND_BAKERY_FORMAT_SOURCE=${SOUND_BAKERY_FORMAT_SOURCE}")
message(STATUS "SOUND_BAKERY_TIDY_SOURCE=${SOUND_BAKERY_TIDY_SOURCE}")

if(SOUND_BAKERY_MAIN_PROJECT)
  include(setup_main_project)
endif()

add_subdirectory(resources)
add_subdirectory(src)
add_subdirectory(apps)

include(setup_tests)