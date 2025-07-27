message(STATUS "Adding Core Library")

set(CORE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/leak_detector.h
    ${CMAKE_CURRENT_SOURCE_DIR}/macros.h
    ${CMAKE_CURRENT_SOURCE_DIR}/core.cpp
)

add_library(sound_bakery_core STATIC ${CORE_FILES})
add_library(SoundBakery::Core ALIAS sound_bakery_core)

c_17(sound_bakery_core)
cxx_20(sound_bakery_core)

target_link_libraries(sound_bakery_core PUBLIC 
  SoundBakery::Delegates 
  Boost::assert
)

target_include_directories(sound_bakery_core PUBLIC ${PROJECT_SOURCE_DIR}/src/)