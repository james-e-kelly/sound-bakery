if (SOUND_BAKERY_EDITOR)
    set(CMAKE_FOLDER apps)

    add_subdirectory(editor)
    add_subdirectory(game)

    # The review app has WIN32-only functionality for the drag and drop, and it
    # is the only consumer of the Tracy profiler GUI. Building that GUI is a
    # heavyweight, platform-specific ExternalProject (and its build byproduct is
    # hardcoded to tracy-profiler.exe), so scope both to Windows. Without this,
    # the profiler ExternalProject - which is part of the default ALL target -
    # would build on Linux/macOS with no consumer and break those CI builds.
    if (WIN32)
        include(build_tracy)
        add_subdirectory(review)
    endif()
endif()