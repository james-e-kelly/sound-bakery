if (${SOUND_BAKERY_EDITOR})
    set(CMAKE_FOLDER apps)
    
    add_subdirectory(editor)
    add_subdirectory(game)

    # The review app has WIN32-only functionality for the drag and drop
    # We don't create it for the other platforms
    if (WIN32)
        add_subdirectory(review)
    endif()
endif()