# Assembles a single THIRD_PARTY_LICENSES file from the licence files of every
# dependency this build actually pulled, so the notices ship with every install /
# release artifact (CI uploads the `dist` install tree).
#
# It scans the roots where dependencies are unpacked - the FetchContent base dir
# (<base>/<name>-src) and the CPM source cache (<cache>/<name>/<hash>) - plus the
# in-repo bundled fonts, and concatenates the top-level LICENSE / COPYING file it
# finds in each. Because it reads whatever is on disk, it always reflects the
# current dependency pins; reconfigure after changing dependencies to refresh it.
#
# sbk_write_third_party_licenses() runs at configure time and must be called after
# all add_subdirectory() calls have populated their dependencies.

if(NOT FETCHCONTENT_BASE_DIR)
    set(FETCHCONTENT_BASE_DIR "${CMAKE_BINARY_DIR}/_deps")
endif()

# Map an on-disk directory name to a friendlier display name where it differs.
macro(_sbk_license_display_name _in _out)
    if("${_in}" STREQUAL "sbk_rpmalloc_content")
        set(${_out} "rpmalloc")
    elseif("${_in}" STREQUAL "bytesizelib")
        set(${_out} "bytesize")
    elseif("${_in}" STREQUAL "magicenum")
        set(${_out} "magic_enum")
    elseif("${_in}" STREQUAL "nfd")
        set(${_out} "nativefiledialog-extended")
    elseif("${_in}" STREQUAL "eabase")
        set(${_out} "EABase")
    elseif("${_in}" STREQUAL "eastl")
        set(${_out} "EASTL")
    elseif("${_in}" STREQUAL "boost-yaml")
        set(${_out} "yaml-archive")
    elseif("${_in}" STREQUAL "sodium-cmake")
        set(${_out} "libsodium")
    elseif("${_in}" STREQUAL "fonts")
        set(${_out} "IconFontCppHeaders")
    else()
        set(${_out} "${_in}")
    endif()
endmacro()

# Append one dependency's licence to the _body / _index accumulators in the
# calling scope. A macro so it can append directly. Does nothing if the directory
# has no recognisable top-level licence file.
macro(_sbk_add_license _dir _display)
    file(GLOB _sbk_lic_files
        "${_dir}/LICENSE*" "${_dir}/LICENCE*" "${_dir}/COPYING*" "${_dir}/UNLICENSE*")
    if(_sbk_lic_files)
        list(GET _sbk_lic_files 0 _sbk_lic_file)
        file(READ "${_sbk_lic_file}" _sbk_lic_text)
        string(APPEND _index "  - ${_display}\n")
        string(APPEND _body
            "\n================================================================================\n"
            "${_display}\n"
            "================================================================================\n\n"
            "${_sbk_lic_text}\n")
    endif()
endmacro()

function(sbk_write_third_party_licenses OUT_VAR)
    cmake_policy(SET CMP0057 NEW)  # IN_LIST operator
    set(_output "${CMAKE_BINARY_DIR}/THIRD_PARTY_LICENSES.txt")

    # Build-only / test-only / tooling dependencies not distributed in binaries.
    set(_exclude cpm doctest catch2 doxygenawesome)

    set(_body "")
    set(_index "")

    # 1. FetchContent dependencies: <base>/<name>-src
    file(GLOB _fc_dirs LIST_DIRECTORIES true "${FETCHCONTENT_BASE_DIR}/*-src")
    foreach(_dir IN LISTS _fc_dirs)
        if(NOT IS_DIRECTORY "${_dir}")
            continue()
        endif()
        get_filename_component(_dirname "${_dir}" NAME)
        string(REGEX REPLACE "-src$" "" _name "${_dirname}")
        if(_name IN_LIST _exclude)
            continue()
        endif()
        _sbk_license_display_name("${_name}" _display)
        _sbk_add_license("${_dir}" "${_display}")
    endforeach()

    # 2. CPM cached dependencies: <cache>/<name>/<hash>
    if(CPM_SOURCE_CACHE)
        file(GLOB _cpm_names LIST_DIRECTORIES true "${CPM_SOURCE_CACHE}/*")
        foreach(_namedir IN LISTS _cpm_names)
            if(NOT IS_DIRECTORY "${_namedir}")
                continue()
            endif()
            get_filename_component(_name "${_namedir}" NAME)
            if(_name IN_LIST _exclude)
                continue()
            endif()
            file(GLOB _hashdirs LIST_DIRECTORIES true "${_namedir}/*")
            foreach(_dir IN LISTS _hashdirs)
                if(IS_DIRECTORY "${_dir}")
                    _sbk_license_display_name("${_name}" _display)
                    _sbk_add_license("${_dir}" "${_display}")
                    break()
                endif()
            endforeach()
        endforeach()
    endif()

    # 3. Bundled in-repo fonts: resources/fonts/<name>
    file(GLOB _font_dirs LIST_DIRECTORIES true "${CMAKE_SOURCE_DIR}/resources/fonts/*")
    foreach(_dir IN LISTS _font_dirs)
        if(IS_DIRECTORY "${_dir}")
            get_filename_component(_name "${_dir}" NAME)
            _sbk_license_display_name("${_name}" _display)
            _sbk_add_license("${_dir}" "${_display}")
        endif()
    endforeach()

    string(TIMESTAMP _now "%Y-%m-%d")
    file(WRITE "${_output}"
        "Sound Bakery - Third-Party Software Licences\n"
        "\n"
        "Sound Bakery is distributed under the MIT License (see the accompanying\n"
        "LICENSE file). It bundles the third-party components listed below, each under\n"
        "its own licence. This file is generated by CMake on ${_now} from the licence\n"
        "files of the dependencies pulled for this build.\n"
        "\n"
        "Bundled components:\n"
        "${_index}"
        "${_body}")

    set(${OUT_VAR} "${_output}" PARENT_SCOPE)
endfunction()
