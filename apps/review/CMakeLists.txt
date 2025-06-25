message(STATUS "Adding Review")

set(BUILD_FOLDER ${CMAKE_BINARY_DIR})

set(review_SOURCES
  app/review_app.cpp
  data/workspace_data.cpp
  managers/project_manager.cpp
  managers/review_manager.cpp
  managers/workspace_manager.cpp
  widgets/review_root_widget.cpp
  widgets/intro_widget.cpp
  widgets/workspace_widget.cpp
)

set(review_HEADERS
  app/review_app.h
  data/workspace_data.h
  managers/project_manager.h
  managers/review_manager.h
  managers/workspace_manager.h
  widgets/review_root_widget.h
  widgets/intro_widget.h
  widgets/workspace_widget.h
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Source Files" FILES ${review_SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Header Files" FILES ${review_HEADERS})

set(PCH_HEADER pch.h)
set(PCH_SOURCE pch.cpp)

add_executable(review 
  ${PCH_SOURCE}
  ${review_SOURCES}
  ${review_HEADERS}
)

set_target_properties(review PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>)

cxx_20(review)

target_include_directories(review
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

target_precompile_headers(review PRIVATE ${PCH_HEADER})

set_target_properties(review PROPERTIES
  OUTPUT_NAME "Sound Check"
)

set_target_properties(review PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER jameskelly.audio
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(review
    PRIVATE
    SoundBakery::Gluten
    SoundBakery::SoundBakery_Shared
)

if(MSVC)
  target_compile_options(review PRIVATE /W4)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
else()
  target_compile_options(review PRIVATE -Wall -Wextra -Wpedantic)
endif()

install(TARGETS review DESTINATION apps)