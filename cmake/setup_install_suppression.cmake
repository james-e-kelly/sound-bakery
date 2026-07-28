# Sound Bakery bundles several third-party libraries whose CMake unconditionally
# install()s their own targets, headers and package-config files. We link them
# statically into our own libraries, so those artifacts have no business in our
# `dist` tree (and some, e.g. Tracy, would even clobber config-specific files at a
# shared path).
#
# None of them expose a per-package "do not install" switch, so we override the
# built-in install() command exactly once with a wrapper that forwards to the real
# command (CMake exposes the overridden builtin as _install) unless the global
# SBK_SUPPRESS_INSTALL flag is set. Raise the flag around a dependency's
# add_subdirectory() / CPMAddPackage() to drop just its install rules; every other
# target still installs normally:
#
#   set_property(GLOBAL PROPERTY SBK_SUPPRESS_INSTALL ON)
#   CPMAddPackage(...)
#   set_property(GLOBAL PROPERTY SBK_SUPPRESS_INSTALL OFF)
#
# This must be included once, early, before any add_subdirectory() that pulls such
# a dependency. Overriding install() a second time would rebind _install to the
# wrapper and silently drop all installs, so this file is the single definition.

set_property(GLOBAL PROPERTY SBK_SUPPRESS_INSTALL OFF)

function(install)
    get_property(_sbk_suppress_install GLOBAL PROPERTY SBK_SUPPRESS_INSTALL)
    if(NOT _sbk_suppress_install)
        _install(${ARGN})
    endif()
endfunction()
