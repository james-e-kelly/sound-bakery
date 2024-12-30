macro(build_nfd)
    message(STATUS "Fetching native file dialog")
    FetchContent_MakeAvailable(nfd)

    if(UNIX AND NOT APPLE)
        set(NFD_SOURCES 
        ${nfd_SOURCE_DIR}/src/nfd_common.c
        ${nfd_SOURCE_DIR}/src/nfd_gtk.c
        ${nfd_SOURCE_DIR}/src/nfd_zenity.c
        )

        find_package(PkgConfig REQUIRED)
        pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
        message("Using GTK version: ${GTK3_VERSION}")
    elseif(APPLE)
        set(NFD_SOURCES 
        ${nfd_SOURCE_DIR}/src/nfd_common.c
        ${nfd_SOURCE_DIR}/src/nfd_cocoa.m
        )
    elseif(WIN32)
        set(NFD_SOURCES 
        ${nfd_SOURCE_DIR}/src/nfd_common.c
        ${nfd_SOURCE_DIR}/src/nfd_win.cpp
        )
    endif()

    add_library(nfd STATIC ${NFD_SOURCES})

    if(APPLE)
        find_library(APPKIT_FRAMEWORK AppKit)
        target_link_libraries(nfd ${APPKIT_FRAMEWORK})
    endif()

    if (UNIX AND NOT APPLE)
        target_include_directories(nfd PRIVATE ${GTK3_INCLUDE_DIRS})
        target_link_libraries(nfd PRIVATE ${GTK3_LINK_LIBRARIES})
    endif()

    target_include_directories(nfd
        PUBLIC ${nfd_SOURCE_DIR}/src/include
        PRIVATE ${nfd_SOURCE_DIR}/src
    )
endmacro()