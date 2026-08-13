set(CMAKE_FOLDER src)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:__cplusplus")
endif()

add_subdirectory(core)
add_subdirectory(sound_chef)
add_subdirectory(sound_bakery)
add_subdirectory(gluten)