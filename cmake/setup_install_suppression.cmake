# Sound Bakery bundles many third-party libraries whose CMake unconditionally
# install()s their own targets, headers and package-config files into standard
# GNUInstallDirs locations (include/, lib/, share/, bin/). We link them statically
# and expose only a curated (C) public API, so none of that belongs in our `dist`
# tree - it just pollutes the release with foreign headers and config.
#
# Rather than opt each dependency out one by one, dependency installs are
# suppressed by DEFAULT: the built-in install() command is overridden with a
# wrapper that does nothing while the global SBK_SUPPRESS_INSTALL flag is set (it
# is, from here on). Sound Bakery's own install rules call sbk_install() instead,
# which forwards to the real command (CMake exposes the overridden builtin as
# _install) regardless of the flag. New dependencies are therefore clean with no
# extra work.
#
#   install(...)        # third-party  -> suppressed
#   sbk_install(...)    # Sound Bakery -> always installed
#
# Included once, early (setup_init), before any add_subdirectory() that pulls a
# dependency. Overriding install() a second time would rebind _install to the
# wrapper and silently drop all installs, so this file is the single definition.

# Default off (transparent: install() forwards to the real command). The
# authoritative value is set from SOUND_BAKERY_INSTALL in the top-level CMakeLists
# once that option exists - it can't be read here, because setup_init (which
# includes this file) runs before the option is defined.
set_property(GLOBAL PROPERTY SBK_SUPPRESS_INSTALL OFF)

function(install)
    get_property(_sbk_suppress_install GLOBAL PROPERTY SBK_SUPPRESS_INSTALL)
    if(NOT _sbk_suppress_install)
        _install(${ARGN})
    endif()
endfunction()

# Some dependencies pair install(TARGETS ... EXPORT foo) with a standalone
# export(EXPORT foo) (e.g. libogg). Suppressing the install() leaves the export set
# undefined, so the bare export() - not an install() - would then error. Override
# export() under the same flag; Sound Bakery never exports a package config itself.
function(export)
    get_property(_sbk_suppress_install GLOBAL PROPERTY SBK_SUPPRESS_INSTALL)
    if(NOT _sbk_suppress_install)
        _export(${ARGN})
    endif()
endfunction()

# Sound Bakery's own install rules: always install, bypassing the suppression flag.
function(sbk_install)
    _install(${ARGN})
endfunction()
