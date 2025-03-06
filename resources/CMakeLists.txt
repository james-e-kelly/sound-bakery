set(CMAKE_FOLDER extern)

message(STATUS "Fetching CMRC for resource embedding")
FetchContent_MakeAvailable(cmrc)

set(CMAKE_FOLDER resources)

add_subdirectory(fonts)
add_subdirectory(images)