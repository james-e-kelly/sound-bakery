include(c_standards)
include(c++_standards)
include(compiler_options)
include(external_dependencies)
include(out_of_source_builds)
include(git_utils)
include(utils)

macro(set_install_path)
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        set(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/dist" CACHE PATH "Sound Bakery Install Prefix" FORCE)
    endif()
    message(STATUS "Install path is ${CMAKE_INSTALL_PREFIX}")
endmacro()

macro(setup_clang_format)
    if (${SOUND_BAKERY_FORMAT_SOURCE})
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

macro(setup_docs)
    if(SOUND_BAKERY_DOCS)
        find_package(Doxygen)
        if(Doxygen_FOUND)
            add_subdirectory(docs)
        else()
            message(WARNING "Doxygen not found, not building docs")
        endif()
    else()
        message(STATUS "Skipping docs")
    endif()
endmacro()

macro(setup_tests)
    if(SOUND_BAKERY_TESTS AND SOUND_BAKERY_MAIN_PROJECT)
        enable_testing()  
        add_subdirectory(tests)
    else()
        message(STATUS "Skipping tests")
    endif()
endmacro()