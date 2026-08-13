message(STATUS "Adding Core Library")

set(CMAKE_FOLDER extern)

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

CPMAddPackage(
    NAME cppdelegates
    GITHUB_REPOSITORY KarateKidzz/CppDelegates
    GIT_TAG 1f6e8c9352ca1c52b2c596520cba2241b9ecee31 # master @ 2026-07-17
    DOWNLOAD_ONLY YES
)

add_library(delegates STATIC 
    ${cppdelegates_SOURCE_DIR}/Delegates.cpp
)

target_include_directories(delegates
    PUBLIC ${cppdelegates_SOURCE_DIR}
)

add_library(SoundBakery::Delegates ALIAS delegates)

c_17(delegates)
cxx_20(delegates)

set(CMAKE_FOLDER src)

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

target_link_libraries(sound_bakery_core PUBLIC
  SoundBakery::BuildConfig
  SoundBakery::Delegates
  Boost::assert
  tl::expected
  spdlog::spdlog
)

target_include_directories(sound_bakery_core PUBLIC ${PROJECT_SOURCE_DIR}/src/)