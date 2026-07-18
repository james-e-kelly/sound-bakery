list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(SOUND_BAKERY_MAIN_PROJECT OFF)
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  set(SOUND_BAKERY_MAIN_PROJECT ON)
endif()

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "In-source builds are not allowed. You should create a separate directory for build files.")
endif()

# fmt marks its format-string-checking constructor `consteval` on compilers it
# believes support it. That decision lives in FMT_CONSTEVAL (fmt/core.h) and is
# keyed off compiler/version detection - NOT the FMT_USE_CONSTEVAL knob, which
# this fmt version does not use at all. Apple Clang on the macOS CI runners
# mis-evaluates that consteval path and fails to compile valid format strings,
# e.g. spdlog's internal SPDLOG_LOGGER_CATCH. Defining FMT_CONSTEVAL as constexpr
# (instead of consteval) lets the check fall back to runtime, sidestepping the
# compiler bug. Global so every translation unit agrees and stays ODR-safe.
add_compile_definitions(FMT_CONSTEVAL=constexpr)

include(FetchContent)
include(fetch_cpm)
include(git_utils)
include(external_dependencies)
include(c_standards)
include(c++_standards)
include(c++_warnings)
include(setup_installation)

set(CPM_SOURCE_CACHE "${CMAKE_BINARY_DIR}/cpm/cache" CACHE PATH "" FORCE)