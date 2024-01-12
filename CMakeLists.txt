# Disable in-source builds to prevent source tree corruption.
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "FATAL: In-source builds are not allowed. You should create a separate directory for build files.")
endif()

cmake_minimum_required(VERSION 3.23)

project(
  SoundBakery 
  VERSION 0.1.0
  DESCRIPTION "An audio middleware solution that takes the best parts of Wwise and FMOD"
)

option(SOUND_BAKERY_DOCS "Build doxygen docs" OFF)
option(SOUND_BAKERY_TESTS "Build test library" OFF)

# Only do these if this is the main project, and not if it is included through add_subdirectory
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)

  set(CMAKE_C_STANDARD 17)
  set (CMAKE_CXX_STANDARD 20)

  set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

  # Let's nicely support folders in IDEs
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)

  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

  set (CPACK_RESOURCE_FILE_LICENSE  
    "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")

  # Testing only available if this is the main app
  # Note this needs to be done in the main CMakeLists
  # since it calls enable_testing, which must be in the
  # main CMakeLists.
  include(CTest)

  if(SOUND_BAKERY_DOCS)
    # Docs only available if this is the main app
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
  # Testing only available if this is the main app
  # Emergency override MODERN_CMAKE_BUILD_TESTING provided as well
  if((CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME OR MODERN_CMAKE_BUILD_TESTING)
    AND BUILD_TESTING)
    add_subdirectory(tests)
  endif()
  else()
    message(STATUS "Skipping tests")
endif()