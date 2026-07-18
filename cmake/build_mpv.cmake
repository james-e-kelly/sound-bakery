include(CMakeDependentOption)
include(ExternalProject)

function(get_mpv_win_dev name)
  ExternalProject_Add(${name}
    URL https://downloads.sourceforge.net/mpv-player-windows/mpv-dev-x86_64-20250713-git-bd21180.7z
    URL_HASH SHA256=e75982d90a1fa3620b194204420386f590b4e96f03dc9daf46b996fccce3549f
    DOWNLOAD_NO_PROGRESS ON
    UPDATE_COMMAND ""
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/include <BINARY_DIR>/include
            COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/libmpv.dll.a <BINARY_DIR>
            COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/libmpv-2.dll <BINARY_DIR>
            COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/libmpv-2.dll ${CMAKE_BINARY_DIR}
    )
  ExternalProject_Get_property(${name} BINARY_DIR)
  set(MPV_DEV_DIR ${BINARY_DIR})

  # Cache variables so both gluten (headers) and any executable linking it (runtime DLL) can see these,
  # regardless of which add_subdirectory() scope calls get_mpv_win_dev().
  set(MPV_INCLUDE_DIRS ${MPV_DEV_DIR}/include CACHE PATH "mpv dev headers" FORCE)
  set(MPV_LIBRARY_DIRS ${MPV_DEV_DIR} CACHE PATH "mpv dev libraries" FORCE)
  set(MPV_LIBRARIES mpv CACHE STRING "mpv library name" FORCE)
endfunction()

cmake_dependent_option(USE_MPV_WIN_BUILD "Use Prebuilt static mpv dll on Windows" ON "WIN32" OFF)

# GLUTEN_ENABLE_VIDEO reflects whether the mpv-backed video subsystem can be
# built. On Windows we always ship the prebuilt dev package. On other platforms
# it depends on libmpv being discoverable via pkg-config; when it isn't, the
# video subsystem is compiled as a no-op stub instead of failing the build.
if(USE_MPV_WIN_BUILD)
  get_mpv_win_dev(mpv_dev)
  set(GLUTEN_ENABLE_VIDEO ON)
else()
  # pkg_search_module requires find_package(PkgConfig) to have run first; it
  # locates pkg-config and initialises the internal state the macro relies on.
  find_package(PkgConfig)

  if(PkgConfig_FOUND)
    pkg_search_module(MPV mpv>=0.33.0)
  endif()

  if(MPV_FOUND)
    set(GLUTEN_ENABLE_VIDEO ON)
  else()
    set(GLUTEN_ENABLE_VIDEO OFF)
    message(STATUS "mpv not found via pkg-config - building without the video subsystem")
  endif()
endif()
