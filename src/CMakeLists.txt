set(CMAKE_FOLDER extern)

CPMAddPackage(
    NAME cppdelegates
    GITHUB_REPOSITORY KarateKidzz/CppDelegates
    GIT_TAG 1f6e8c9352ca1c52b2c596520cba2241b9ecee31 # master @ 2026-07-17
    DOWNLOAD_ONLY YES
)

CPMAddPackage(
    NAME stb
    GITHUB_REPOSITORY nothings/stb
    GIT_TAG 31c1ad37456438565541f4919958214b6e762fb4 # master @ 2026-07-17
    DOWNLOAD_ONLY YES
)

add_library(delegates STATIC 
    ${cppdelegates_SOURCE_DIR}/Delegates.cpp
)

target_include_directories(delegates
    PUBLIC ${cppdelegates_SOURCE_DIR}
)

c_17(delegates)
cxx_20(delegates)

add_library(SoundBakery::Delegates ALIAS delegates)

set(CMAKE_FOLDER src)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:__cplusplus")
endif()

add_subdirectory(core)
add_subdirectory(sound_chef)
add_subdirectory(sound_bakery)
add_subdirectory(gluten)