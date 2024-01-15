if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(FATAL_ERROR "In-source builds are not allowed. You should create a separate directory for build files.")
endif()

cmake_minimum_required(VERSION 3.23)

project(SoundBakery VERSION 0.1.0 DESCRIPTION "An open-source audio middleware tool with an authoring application and programmer's API")

set(SOUND_BAKERY_MAIN_PROJECT OFF)
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
  set(SOUND_BAKERY_MAIN_PROJECT ON)
endif()

option(SOUND_BAKERY_DOCS "Build doxygen docs" OFF)
option(SOUND_BAKERY_TESTS "Build test library" OFF)
option(SOUND_BAKERY_FORMAT_SOURCE "Format source" ${SOUND_BAKERY_MAIN_PROJECT})
option(SOUND_BAKERY_TIDY_SOURCE "Run clang-tidy on source files" OFF)

message(STATUS "SOUND_BAKERY_MAIN_PROJECT=${SOUND_BAKERY_MAIN_PROJECT}")
message(STATUS "SOUND_BAKERY_DOCS=${SOUND_BAKERY_DOCS}")
message(STATUS "SOUND_BAKERY_TESTS=${SOUND_BAKERY_TESTS}")
message(STATUS "SOUND_BAKERY_FORMAT_SOURCE=${SOUND_BAKERY_FORMAT_SOURCE}")
message(STATUS "SOUND_BAKERY_TIDY_SOURCE=${SOUND_BAKERY_TIDY_SOURCE}")

if(SOUND_BAKERY_MAIN_PROJECT)
  set(CMAKE_C_STANDARD 17)
  set(CMAKE_CXX_STANDARD 20)

  set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

  set_property(GLOBAL PROPERTY USE_FOLDERS ON)

  set (CPACK_RESOURCE_FILE_LICENSE  
    "${CMAKE_SOURCE_DIR}/LICENSE")

  if (${SOUND_BAKERY_FORMAT_SOURCE})
    find_program(SOUND_BAKERY_CLANG_FORMAT_EXE NAMES clang-format)
  endif()

  if(DEFINED SOUND_BAKERY_CLANG_FORMAT_EXE)
    message(STATUS "Found clang-format exe")
  endif()

  if(SOUND_BAKERY_TIDY_SOURCE)
    find_program(SOUND_BAKERY_CLANG_TIDY_EXE NAMES clang-tidy)
  endif()

  if(DEFINED SOUND_BAKERY_TIDY_SOURCE)
    message(STATUS "Found clang-tidy exe")
  endif()

  include(CTest)

  if(SOUND_BAKERY_DOCS)
    find_package(Doxygen)
    if(Doxygen_FOUND)
      add_subdirectory(docs)
    else()
      message(WARNING "Doxygen not found, not building docs")
    endif()
  else()
    message(STATUS "Skipping docs")
  endif()
endif()

include(FetchContent)

set(CMAKE_FOLDER src)
add_subdirectory(src)
add_subdirectory(resources)
set(CMAKE_FOLDER apps)
add_subdirectory(apps)

if(SOUND_BAKERY_TESTS)
  if((CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME OR MODERN_CMAKE_BUILD_TESTING) AND BUILD_TESTING)
    add_subdirectory(tests)
  endif()
  else()
    message(STATUS "Skipping tests")
endif()