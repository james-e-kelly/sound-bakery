macro(fix_linux_clang)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND UNIX AND NOT APPLE)
        # Clang on Linux needs to use its own standard library headers and not the system default (gcc)
        # https://stackoverflow.com/questions/76859275/error-compiling-a-cpp-containing-stdchrono-errorstatic-constexpr-unsigned-fra
        string(APPEND CMAKE_CXX_FLAGS " -stdlib=libc++")
    endif()
endmacro()

macro(detect_main_project)
    if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
        set(SOUND_BAKERY_MAIN_PROJECT ON)
    else()
        set(SOUND_BAKERY_MAIN_PROJECT OFF)
    endif()
endmacro()