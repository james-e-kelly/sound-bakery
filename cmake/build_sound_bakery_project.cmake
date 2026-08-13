include_guard(GLOBAL)

# Adds a `format_<name>` custom target that runs clang-format over the given
# sources. No-op unless SOUND_BAKERY_FORMAT_SOURCE is on and clang-format was
# located. Relative paths are resolved against CMAKE_CURRENT_SOURCE_DIR.
function(sbk_add_format_target)
    cmake_parse_arguments(ARG "" "NAME" "SOURCES" ${ARGN})
    if(NOT (SOUND_BAKERY_FORMAT_SOURCE AND SOUND_BAKERY_CLANG_FORMAT_EXE))
        return()
    endif()
    set(files "")
    foreach(source IN LISTS ARG_SOURCES)
        if(IS_ABSOLUTE "${source}")
            list(APPEND files "${source}")
        else()
            list(APPEND files "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
        endif()
    endforeach()
    add_custom_target(format_${ARG_NAME}
        COMMAND clang-format --style=file -i ${files}
        COMMAND_EXPAND_LISTS
        COMMENT "Running clang-format"
        VERBATIM
    )
endfunction()

macro(setup_clang_format)
    if (SOUND_BAKERY_FORMAT_SOURCE)
        find_program(SOUND_BAKERY_CLANG_FORMAT_EXE NAMES clang-format)
    endif()

    if(SOUND_BAKERY_CLANG_FORMAT_EXE)
        message(STATUS "Found clang-format exe")
    endif()
endmacro()

macro(setup_clang_tidy)
    if(SOUND_BAKERY_TIDY_SOURCE)
        set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
        find_program(SOUND_BAKERY_CLANG_TIDY_EXE NAMES clang-tidy)

        if(SOUND_BAKERY_CLANG_TIDY_EXE)
            message(STATUS "Found clang-tidy exe")
        else()
            message(FATAL_ERROR "Could not find clang-tidy exe")
        endif()
    endif()
endmacro()