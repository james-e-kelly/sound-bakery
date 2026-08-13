message(STATUS "Adding Sound Bakery")
include(build_sound_bakery) # Split into separate files to make it easier to read
set_sources()

configure_file (${CMAKE_CURRENT_SOURCE_DIR}/sound_bakery_version.h.in ${PROJECT_SOURCE_DIR}/inc/sound_bakery/sound_bakery_version.h @ONLY)

add_library(sound_bakery_shared SHARED ${SOUND_BAKERY_SOURCES} ${SOUND_BAKERY_HEADERS} ${PCH_SOURCE})
add_library(sound_bakery_static STATIC ${SOUND_BAKERY_SOURCES} ${SOUND_BAKERY_HEADERS} ${PCH_SOURCE})

foreach(t IN ITEMS sound_bakery_shared sound_bakery_static)
    c_17(${t})
    cxx_20(${t})
    set_target_properties(${t} PROPERTIES LANGUAGE CXX)
    target_precompile_headers(${t} PRIVATE ${PCH_HEADER})
    target_compile_options(${t} PRIVATE ${PROJECT_LIBRARY_WARNING_FLAGS})
    target_include_directories(${t}
        PUBLIC ${PROJECT_SOURCE_DIR}/src/
        PUBLIC ${PROJECT_SOURCE_DIR}/inc/
    )
endforeach()

build_dependencies()
setup_format_sources()
setup_linter_targets()
fix_msvc_linters()

target_link_libraries(sound_bakery_static PUBLIC
  SoundBakery::BuildConfig
  SoundBakery::Core
  SoundBakery::SoundChef_Static
  RTTR::Core_Lib
  Boost::serialization
  Boost::msm
  yaml-archive
  Tracy::TracyClient
  EASTL
  PRIVATE
  sbk::rpmalloc
)

target_link_libraries(sound_bakery_shared PUBLIC
  SoundBakery::BuildConfig
  SoundBakery::Core
  SoundBakery::SoundChef_Shared
  RTTR::Core_Lib
  spdlog::spdlog
  Boost::serialization
  Boost::msm
  yaml-archive
  Tracy::TracyClient
  EASTL
  PRIVATE
  sbk::rpmalloc
)

if (SOUND_BAKERY_INSTALL)
foreach(t IN ITEMS sound_bakery_shared sound_bakery_static)
    sbk_install(TARGETS ${t} DESTINATION lib/${SBK_INSTALL_CONFIG_SUBDIR}/sound_bakery)
endforeach()
sbk_install(FILES
  ${PROJECT_SOURCE_DIR}/inc/sound_bakery/sound_bakery_common.h
  ${PROJECT_SOURCE_DIR}/inc/sound_bakery/sound_bakery_version.h
  ${PROJECT_SOURCE_DIR}/inc/sound_bakery/sound_bakery.h
  DESTINATION inc/sound_bakery)
endif()

add_library(SoundBakery::SoundBakery_Shared ALIAS sound_bakery_shared)
add_library(SoundBakery::SoundBakery_Static ALIAS sound_bakery_static)