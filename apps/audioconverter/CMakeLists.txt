CPMAddPackage(
    NAME miniaudio
    GITHUB_REPOSITORY mackron/miniaudio
    GIT_TAG 0.11.21
    DOWNLOAD_ONLY YES
)

add_executable(audioconverter ${miniaudio_SOURCE_DIR}/tools/audioconverter/audioconverter.c)
target_include_directories(audioconverter PRIVATE ${miniaudio_SOURCE_DIR})

find_library(MATH_LIBRARY m)
if(MATH_LIBRARY)
    target_link_libraries(audioconverter PUBLIC ${MATH_LIBRARY})
endif()
