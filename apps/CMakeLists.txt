if (${SOUND_BAKERY_EDITOR})
    set(CMAKE_FOLDER apps)
    
    add_subdirectory(editor)
    add_subdirectory(game)
    add_subdirectory(review)
endif()