CPMAddPackage(
    NAME doctest
    GITHUB_REPOSITORY doctest/doctest
    GIT_TAG v2.4.11
    EXCLUDE_FROM_ALL YES
)
include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)

add_executable(sound_bakery_test test_sound_bakery.cpp)
target_link_libraries(sound_bakery_test PRIVATE SoundBakery::SoundBakery_Static doctest::doctest)
doctest_discover_tests(sound_bakery_test)

cxx_20(sound_bakery_test)