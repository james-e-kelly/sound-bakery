message(STATUS "Adding Sound Bakery")
include(build_sound_bakery) # Split into separate files to make it easier to read
set_sources()

configure_file (${CMAKE_CURRENT_SOURCE_DIR}/sound_bakery_version.h.in ${PROJECT_SOURCE_DIR}/inc/sound_bakery/sound_bakery_version.h @ONLY)

add_library(sound_bakery_shared SHARED ${SOUND_BAKERY_SOURCES} ${SOUND_BAKERY_HEADERS} ${PCH_SOURCE})
add_library(sound_bakery_static STATIC ${SOUND_BAKERY_SOURCES} ${SOUND_BAKERY_HEADERS} ${PCH_SOURCE})

c_17(sound_bakery_shared)
c_17(sound_bakery_static)

cxx_20(sound_bakery_shared)
cxx_20(sound_bakery_static)

set_target_properties(sound_bakery_shared PROPERTIES LANGUAGE CXX)
set_target_properties(sound_bakery_static PROPERTIES LANGUAGE CXX)

set_target_properties(sound_bakery_shared PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>)

target_precompile_headers(sound_bakery_shared PRIVATE ${PCH_HEADER})
target_precompile_headers(sound_bakery_static PRIVATE ${PCH_HEADER})

target_compile_options(sound_bakery_shared PRIVATE ${PROJECT_LIBRARY_WARNING_FLAGS})
target_compile_options(sound_bakery_static PRIVATE ${PROJECT_LIBRARY_WARNING_FLAGS})

target_include_directories(sound_bakery_shared
    PUBLIC ${PROJECT_SOURCE_DIR}/src/
    PUBLIC ${PROJECT_SOURCE_DIR}/inc/
)

target_include_directories(sound_bakery_static
    PUBLIC ${PROJECT_SOURCE_DIR}/src/
    PUBLIC ${PROJECT_SOURCE_DIR}/inc/
)

build_dependencies()
set_big_objects()
setup_format_sources()
setup_linter_targets()
fix_msvc_linters()

target_link_libraries(sound_bakery_static PUBLIC 
  SoundBakery::Core
  SoundBakery::SoundChef_Static
  RTTR::Core_Lib
  concurrencpp::concurrencpp
  Boost::serialization
  Boost::msm
  ztd::out_ptr
  yaml-archive
  Tracy::TracyClient
  PRIVATE
  sbk::rpmalloc
)

target_link_libraries(sound_bakery_shared PUBLIC 
  SoundBakery::Core
  SoundBakery::SoundChef_Shared
  RTTR::Core_Lib
  concurrencpp::concurrencpp
  spdlog::spdlog
  Boost::serialization
  Boost::msm
  ztd::out_ptr
  yaml-archive
  Tracy::TracyClient
  PRIVATE
  sbk::rpmalloc
)

install(TARGETS sound_bakery_shared DESTINATION lib/${SBK_INSTALL_CONFIG_SUBDIR}/sound_bakery)
install(TARGETS sound_bakery_static DESTINATION lib/${SBK_INSTALL_CONFIG_SUBDIR}/sound_bakery)
install(FILES 
  ${PROJECT_SOURCE_DIR}/inc/sound_bakery/sound_bakery_common.h    
  ${PROJECT_SOURCE_DIR}/inc/sound_bakery/sound_bakery_version.h 
  ${PROJECT_SOURCE_DIR}/inc/sound_bakery/sound_bakery.h
  DESTINATION inc/sound_bakery)

add_library(SoundBakery::SoundBakery_Shared ALIAS sound_bakery_shared)
add_library(SoundBakery::SoundBakery_Static ALIAS sound_bakery_static)