if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/dist" CACHE PATH "Sound Bakery Install Prefix" FORCE)
endif()
message(STATUS "Install path is ${CMAKE_INSTALL_PREFIX}")

# Wwise-style per-configuration install layout: binaries land in a folder named
# after the configuration, so Debug / Profile / Release builds can be installed
# side by side into one `dist` tree without clobbering each other. A consuming
# game references the folder matching its own configuration (e.g. via
# $(Configuration) / $<CONFIG>) and links the ABI-compatible library.
#
# The subdir uses Sound Bakery's semantic names, not the raw CMake config names,
# to match SoundBakery::BuildConfig (see setup_build_config.cmake):
#
#   CMake config    | Install folder
#   --------------- | --------------
#   Debug           | Debug
#   RelWithDebInfo  | Profile
#   Release         | Release
#   MinSizeRel      | Release   (macro-identical to Release)
#
# This is a generator expression, so it resolves per-config at install time and
# works with both multi-config (Visual Studio) and single-config generators.
# Headers are configuration-independent and are installed flat, outside this dir.
set(SBK_INSTALL_CONFIG_SUBDIR
    "$<IF:$<CONFIG:RelWithDebInfo>,Profile,$<IF:$<CONFIG:MinSizeRel>,Release,$<CONFIG>>>")
