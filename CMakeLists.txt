if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(FATAL_ERROR "In-source builds are not allowed. You should create a separate directory for build files.")
endif()

# Version set to have EXCLUDE_FROM_ALL available in FetchContent_Declare
cmake_minimum_required(VERSION 3.28)

project(SoundBakery VERSION 0.1.0 DESCRIPTION "An open-source audio middleware tool with an authoring application and programmer's API")

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/dist" CACHE PATH "Sound Bakery Install Prefix" FORCE)
endif()

message(STATUS "Install path is ${CMAKE_INSTALL_PREFIX}")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(external_dependencies)

set(SOUND_BAKERY_MAIN_PROJECT OFF)
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
  set(SOUND_BAKERY_MAIN_PROJECT ON)
endif()

option(SOUND_BAKERY_SHARED "Build Sound Bakery and Sound Chef as shared libraries" OFF)
option(SOUND_BAKERY_DOCS "Build doxygen docs" OFF)
option(SOUND_BAKERY_TESTS "Build test library" OFF)
option(SOUND_BAKERY_FORMAT_SOURCE "Format source" ${SOUND_BAKERY_MAIN_PROJECT})
option(SOUND_BAKERY_TIDY_SOURCE "Run clang-tidy on source files" OFF)

message(STATUS "SOUND_BAKERY_MAIN_PROJECT=${SOUND_BAKERY_MAIN_PROJECT}")
message(STATUS "SOUND_BAKERY_SHARED=${SOUND_BAKERY_SHARED}")
message(STATUS "SOUND_BAKERY_DOCS=${SOUND_BAKERY_DOCS}")
message(STATUS "SOUND_BAKERY_TESTS=${SOUND_BAKERY_TESTS}")
message(STATUS "SOUND_BAKERY_FORMAT_SOURCE=${SOUND_BAKERY_FORMAT_SOURCE}")
message(STATUS "SOUND_BAKERY_TIDY_SOURCE=${SOUND_BAKERY_TIDY_SOURCE}")

if(SOUND_BAKERY_MAIN_PROJECT)
  set(CMAKE_C_STANDARD 17)
  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_EXTENSIONS OFF)

  if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND UNIX AND NOT APPLE)
    # Clang on Linux needs to use its own standard library headers and not the system default (gcc)
    # https://stackoverflow.com/questions/76859275/error-compiling-a-cpp-containing-stdchrono-errorstatic-constexpr-unsigned-fra
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
  endif()

  set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

  set_property(GLOBAL PROPERTY USE_FOLDERS ON)

  set(CMAKE_POSITION_INDEPENDENT_CODE ON)

  set (CPACK_RESOURCE_FILE_LICENSE  
    "${CMAKE_SOURCE_DIR}/LICENSE")

  if (${SOUND_BAKERY_FORMAT_SOURCE})
    find_program(SOUND_BAKERY_CLANG_FORMAT_EXE NAMES clang-format)
  endif()

  if(SOUND_BAKERY_CLANG_FORMAT_EXE)
    message(STATUS "Found clang-format exe")
  endif()

  if(SOUND_BAKERY_TIDY_SOURCE)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    find_program(SOUND_BAKERY_CLANG_TIDY_EXE NAMES clang-tidy)

    if(SOUND_BAKERY_CLANG_TIDY_EXE)
      message(STATUS "Found clang-tidy exe")
    else()
      message(FATAL_ERROR "Could not find clang-tidy exe")
    endif()
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

set(CMAKE_FOLDER src)
add_subdirectory(src)
set(CMAKE_FOLDER apps)
add_subdirectory(apps)
set(CMAKE_FOLDER resources)
add_subdirectory(resources)

if(SOUND_BAKERY_TESTS)
  if((CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME OR MODERN_CMAKE_BUILD_TESTING) AND BUILD_TESTING)
    add_subdirectory(tests)
  endif()
  else()
    message(STATUS "Skipping tests")
endif()