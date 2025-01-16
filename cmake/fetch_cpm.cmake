include(FetchContent)

FetchContent_Declare(
    cpm
    GIT_REPOSITORY https://github.com/cpm-cmake/CPM.cmake.git
    GIT_TAG v0.40.5
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(cpm)

include(${cpm_SOURCE_DIR}/cmake/CPM.cmake)