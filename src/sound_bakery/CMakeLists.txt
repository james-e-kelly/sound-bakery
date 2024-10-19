message(STATUS "Adding Sound Bakery")
include(build_sound_bakery) # Split into separate files to make it easier to read
set_sources()

configure_file (${CMAKE_CURRENT_SOURCE_DIR}/sound_bakery_version.h.in ${CMAKE_SOURCE_DIR}/inc/sound_bakery/sound_bakery_version.h @ONLY)

# We do this for doctest https://github.com/doctest/doctest/blob/master/doc/markdown/faq.md#why-are-my-tests-in-a-static-library-not-getting-registered
add_library(sound_bakery_object OBJECT ${SOUND_BAKERY_SOURCES} ${SOUND_BAKERY_HEADERS} ${PCH_SOURCE})
set_target_properties(sound_bakery_object PROPERTIES LANGUAGE CXX)
target_precompile_headers(sound_bakery_object PRIVATE ${PCH_HEADER})

target_include_directories(sound_bakery_object
    PUBLIC ${CMAKE_SOURCE_DIR}/src/
    PUBLIC ${CMAKE_SOURCE_DIR}/inc/
)

build_dependencies()
set_big_objects()
setup_format_sources()
setup_linter_targets()
fix_msvc_linters()

if (SOUND_BAKERY_SHARED)
  set(SOUND_BAKERY_BUILD SHARED)
else()
  set(SOUND_BAKERY_BUILD STATIC)
endif()

crt_registertest(
  TEST_INPLACE
  INPUT_OBJECT_LIBRARY sound_bakery_object
  OUTPUT_LIBRARY_NAME sound_bakery
  OUTPUT_LIBRARY_TYPE ${SOUND_BAKERY_BUILD}
  OUTPUT_TEST_TARGET sound_bakery_test
)

add_library(SoundBakery::SoundBakery ALIAS sound_bakery)

target_compile_options(sound_bakery PRIVATE ${PROJECT_WARNING_FLAGS})

target_link_libraries(sound_bakery PUBLIC 
  SoundBakery::Delegates 
  SoundBakery::SoundChef
  RTTR::Core_Lib
  concurrencpp::concurrencpp
  spdlog::spdlog
  yaml-cpp::yaml-cpp
  doctest::doctest)

if (SOUND_BAKERY_TESTS)

endif()

install(TARGETS sound_bakery DESTINATION lib)