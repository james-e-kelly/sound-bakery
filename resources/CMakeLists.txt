set(CMAKE_FOLDER extern)

CPMAddPackage(
  NAME cmrc
  GITHUB_REPOSITORY vector-of-bool/cmrc
  GIT_TAG 2.0.1
  EXCLUDE_FROM_ALL YES
)

set(CMAKE_FOLDER resources)

add_subdirectory(fonts)
add_subdirectory(images)