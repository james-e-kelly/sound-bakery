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
include(external_dependencies)
include(c_standards)
include(c++_standards)
include(c++_warnings)