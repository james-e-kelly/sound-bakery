list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(SOUND_BAKERY_MAIN_PROJECT OFF)
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  set(SOUND_BAKERY_MAIN_PROJECT ON)
endif()

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "In-source builds are not allowed. You should create a separate directory for build files.")
endif()

include(FetchContent)
include(fetch_cpm)
include(git_utils)
include(c_standards)
include(c++_standards)
include(c++_warnings)
include(setup_build_config)
include(setup_installation)
include(setup_install_suppression)

set(CPM_SOURCE_CACHE "${CMAKE_BINARY_DIR}/cpm/cache" CACHE PATH "" FORCE)

# CMake 4.0 removed compatibility with cmake_minimum_required(VERSION < 3.5).
# Some pinned third-party deps (e.g. cmrc 2.0.1, which is unmaintained upstream)
# still declare older minimums, so allow them to configure under CMake 4.x.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "Minimum CMake policy version for old dependencies")