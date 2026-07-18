list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(SOUND_BAKERY_MAIN_PROJECT OFF)
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  set(SOUND_BAKERY_MAIN_PROJECT ON)
endif()

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "In-source builds are not allowed. You should create a separate directory for build files.")
endif()

# fmt enables consteval (compile-time) format-string checking on compilers it
# believes support it. Some toolchains - notably Apple Clang on the macOS CI
# runners - mis-evaluate fmt's consteval checker and fail to compile valid
# format strings, e.g. spdlog's internal SPDLOG_LOGGER_CATCH. Forcing the check
# to run at runtime sidesteps the compiler bug. Set globally so every fmt copy
# (the standalone fmt target and spdlog's bundled fmt) agrees and stays ODR-safe.
add_compile_definitions(FMT_USE_CONSTEVAL=0)

include(FetchContent)
include(fetch_cpm)
include(git_utils)
include(external_dependencies)
include(c_standards)
include(c++_standards)
include(c++_warnings)
include(setup_installation)

set(CPM_SOURCE_CACHE "${CMAKE_BINARY_DIR}/cpm/cache" CACHE PATH "" FORCE)