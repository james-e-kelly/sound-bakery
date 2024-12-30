FetchContent_MakeAvailable(doctest)
include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)

add_executable(sound_bakery_test test_sound_bakery.cpp)
target_link_libraries(sound_bakery_test PRIVATE SoundBakery::SoundBakery doctest::doctest)
doctest_discover_tests(sound_bakery_test)