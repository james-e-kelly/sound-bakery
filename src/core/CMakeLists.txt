message(STATUS "Adding Core Library")

set(CORE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/enum_flags.h
    ${CMAKE_CURRENT_SOURCE_DIR}/leak_detector.h
    ${CMAKE_CURRENT_SOURCE_DIR}/macros.h
    ${CMAKE_CURRENT_SOURCE_DIR}/logger.h
    ${CMAKE_CURRENT_SOURCE_DIR}/core.cpp
)

add_library(sound_bakery_core STATIC ${CORE_FILES})
add_library(SoundBakery::Core ALIAS sound_bakery_core)

c_17(sound_bakery_core)
cxx_20(sound_bakery_core)

# We only use tl::expected's header-only interface library. Left to itself it
# runs include(CTest) (turning BUILD_TESTING on), then builds a test executable
# that links Catch2::Catch2 - which we don't provide, breaking generation. Disable
# its tests and packaging so only the interface target is configured. (Its own
# install rules are suppressed by default; see setup_install_suppression.cmake.)
CPMAddPackage(
  NAME expected
  GITHUB_REPOSITORY TartanLlama/expected
  VERSION 1.2.0
  OPTIONS
    "EXPECTED_BUILD_TESTS OFF"
    "EXPECTED_BUILD_PACKAGE OFF"
)

target_link_libraries(sound_bakery_core PUBLIC
  SoundBakery::BuildConfig
  SoundBakery::Delegates
  Boost::assert
  tl::expected
  spdlog::spdlog
)

target_include_directories(sound_bakery_core PUBLIC ${PROJECT_SOURCE_DIR}/src/)