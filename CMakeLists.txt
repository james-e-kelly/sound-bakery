cmake_minimum_required(VERSION 3.28) # Version set to have EXCLUDE_FROM_ALL available in FetchContent_Declare

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(fetch_cpm)
include(build_sound_bakery_project)

project("Sound Bakery" VERSION 0.2.0 DESCRIPTION "An open-source audio middleware tool with an authoring application and programmer's API")

disallow_source_builds()
get_commit_id()
set_install_path()
detect_main_project()

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
message(STATUS "SOUND_BAKERY_DOCS=${SOUND_BAKERY_DOCS}")
message(STATUS "SOUND_BAKERY_TESTS=${SOUND_BAKERY_TESTS}")
message(STATUS "SOUND_BAKERY_FORMAT_SOURCE=${SOUND_BAKERY_FORMAT_SOURCE}")
message(STATUS "SOUND_BAKERY_TIDY_SOURCE=${SOUND_BAKERY_TIDY_SOURCE}")

if(SOUND_BAKERY_MAIN_PROJECT)
  c_17()
  cxx_20()

  #fix_linux_clang()

  set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
  set (CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)

  include(CPack)

  setup_clang_format()
  setup_clang_tidy()
  setup_docs()
endif()

set(CMAKE_FOLDER src)
add_subdirectory(src)

if (${SOUND_BAKERY_EDITOR})
  set(CMAKE_FOLDER apps)
  add_subdirectory(apps)
endif()

set(CMAKE_FOLDER resources)
add_subdirectory(resources)

setup_tests()