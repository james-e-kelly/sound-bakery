message(STATUS "Adding Sound Bakery")
include(build_sound_bakery) # Split into separate files to make it easier to read
set_sources()

configure_file (${CMAKE_CURRENT_SOURCE_DIR}/sound_bakery_version.h.in ${CMAKE_SOURCE_DIR}/inc/sound_bakery/sound_bakery_version.h @ONLY)

if (SOUND_BAKERY_SHARED)
  add_library(sound_bakery SHARED ${SOUND_BAKERY_SOURCES} ${SOUND_BAKERY_HEADERS} ${PCH_SOURCE})
else()
  add_library(sound_bakery STATIC ${SOUND_BAKERY_SOURCES} ${SOUND_BAKERY_HEADERS} ${PCH_SOURCE})
endif()

add_library(SoundBakery::SoundBakery ALIAS sound_bakery)

set_target_properties(sound_bakery PROPERTIES LANGUAGE CXX)
target_precompile_headers(sound_bakery PRIVATE ${PCH_HEADER})
target_compile_options(sound_bakery PRIVATE ${PROJECT_WARNING_FLAGS})
target_compile_definitions(sound_bakery PUBLIC TRACY_ENABLE TRACY_ON_DEMAND TRACY_NO_VSYNC_CAPTURE TRACY_NO_FRAME_IMAGE)
target_include_directories(sound_bakery
    PUBLIC ${CMAKE_SOURCE_DIR}/src/
    PUBLIC ${CMAKE_SOURCE_DIR}/inc/
)

build_dependencies()
set_big_objects()
setup_format_sources()
setup_linter_targets()
fix_msvc_linters()

target_link_libraries(sound_bakery PUBLIC 
  SoundBakery::Delegates 
  SoundBakery::SoundChef
  RTTR::Core_Lib
  concurrencpp::concurrencpp
  spdlog::spdlog
  Boost::assert
  Boost::serialization
  Boost::msm
  ztd::out_ptr
  yaml-archive
  Tracy::TracyClient
)

install(TARGETS sound_bakery DESTINATION lib/sound_bakery)
install(FILES 
  ${CMAKE_SOURCE_DIR}/inc/sound_bakery/sound_bakery_common.h    
  ${CMAKE_SOURCE_DIR}/inc/sound_bakery/sound_bakery_version.h 
  ${CMAKE_SOURCE_DIR}/inc/sound_bakery/sound_bakery.h
  DESTINATION inc/sound_bakery)