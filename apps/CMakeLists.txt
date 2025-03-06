if (${SOUND_BAKERY_EDITOR})
    set(CMAKE_FOLDER apps)
    
    add_subdirectory(editor)
    add_subdirectory(game)
endif()
