# Wwise/FMOD-style build configurations for Sound Chef and Sound Bakery.
#
# Three configurations, mapped onto CMake's per-config generator expressions so
# they work with multi-config generators (Visual Studio) and single-config alike:
#
#   Config  | CMake configuration    | Meaning
#   ------- | ---------------------- | -------------------------------------------------
#   Debug   | Debug                  | Full checks, logging, profiling/remote connections
#   Profile | RelWithDebInfo         | Optimized but instrumented - logging and profiling
#           |                        | stay on (Wwise "Profile", FMOD "L" logging builds)
#   Release | Release / MinSizeRel   | Optimized, logging and profiling compiled out
#
# Every macro is always defined to 0 or 1, so code uses `#if SBK_CONFIG_DEBUG`
# (not `#ifdef`) and a missing include shows up as a compile error rather than
# silently taking the release path. Definitions ride on SoundBakery::BuildConfig,
# an INTERFACE target linked PUBLIC into the core, Sound Chef, and Sound Bakery
# targets, so both the C and C++ layers (and anything linking them) see them.
#
# Independent overrides, FMOD-style (a logging release build is legal):
#   -DSOUND_BAKERY_LOGGING=ON|OFF|AUTO      (AUTO: on in Debug/Profile, off in Release)
#   -DSOUND_BAKERY_PROFILING=ON|OFF|AUTO    (AUTO: on in Debug/Profile, off in Release)

set(SOUND_BAKERY_LOGGING "AUTO" CACHE STRING "Compile logging in: AUTO (Debug/Profile only), ON, or OFF")
set(SOUND_BAKERY_PROFILING "AUTO" CACHE STRING "Compile profiling/remote connections in: AUTO (Debug/Profile only), ON, or OFF")
set_property(CACHE SOUND_BAKERY_LOGGING PROPERTY STRINGS AUTO ON OFF)
set_property(CACHE SOUND_BAKERY_PROFILING PROPERTY STRINGS AUTO ON OFF)

add_library(sound_bakery_build_config INTERFACE)
add_library(SoundBakery::BuildConfig ALIAS sound_bakery_build_config)

# 1 in exactly one of the three configurations.
set(SBK_GENEX_DEBUG "$<CONFIG:Debug>")
set(SBK_GENEX_PROFILE "$<CONFIG:RelWithDebInfo>")
set(SBK_GENEX_RELEASE "$<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>>")

# AUTO means "everything except Release".
set(SBK_GENEX_NOT_RELEASE "$<NOT:${SBK_GENEX_RELEASE}>")

if(SOUND_BAKERY_LOGGING STREQUAL "ON")
  set(SBK_GENEX_LOGGING "1")
elseif(SOUND_BAKERY_LOGGING STREQUAL "OFF")
  set(SBK_GENEX_LOGGING "0")
else()
  set(SBK_GENEX_LOGGING "${SBK_GENEX_NOT_RELEASE}")
endif()

if(SOUND_BAKERY_PROFILING STREQUAL "ON")
  set(SBK_GENEX_PROFILING "1")
elseif(SOUND_BAKERY_PROFILING STREQUAL "OFF")
  set(SBK_GENEX_PROFILING "0")
else()
  set(SBK_GENEX_PROFILING "${SBK_GENEX_NOT_RELEASE}")
endif()

target_compile_definitions(sound_bakery_build_config INTERFACE
  SBK_HAS_BUILD_CONFIG=1
  SBK_CONFIG_DEBUG=${SBK_GENEX_DEBUG}
  SBK_CONFIG_PROFILE=${SBK_GENEX_PROFILE}
  SBK_CONFIG_RELEASE=${SBK_GENEX_RELEASE}
  SBK_CONFIG_ENABLE_LOGGING=${SBK_GENEX_LOGGING}
  SBK_CONFIG_ENABLE_PROFILING=${SBK_GENEX_PROFILING}
)

if (SOUND_BAKERY_PROFILING)
  target_compile_definitions(sound_bakery_build_config INTERFACE
    TRACY_ENABLE
  )
endif()

message(STATUS "SOUND_BAKERY_LOGGING=${SOUND_BAKERY_LOGGING}")
message(STATUS "SOUND_BAKERY_PROFILING=${SOUND_BAKERY_PROFILING}")