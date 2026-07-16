message(STATUS "Adding Review")

set(BUILD_FOLDER ${CMAKE_BINARY_DIR})

set(review_SOURCES
  app/review_app.cpp
  app/review_database.cpp
  app/review_client.cpp
  app/review_server.cpp
  data/activity_data.cpp
  data/comment_data.cpp
  data/user_data.cpp
  data/workspace_data.cpp
  data/review_data.cpp
  elements/inline_user_display_element.cpp
  elements/review_element.cpp
  elements/project_element.cpp
  elements/video_element.cpp
  elements/audio_element.cpp
  elements/file_element.cpp
  glad/src/gl.cpp
  managers/project_manager.cpp
  managers/review_manager.cpp
  managers/workspace_manager.cpp
  managers/intro_manager.cpp
  subsystems/video_subsystem.cpp
  widgets/review_root_widget.cpp
  widgets/intro_widget.cpp
  widgets/workspace_widget.cpp
  widgets/create_comment_popup.cpp
  widgets/create_project_popup.cpp
  widgets/create_review_popup.cpp
  widgets/update_review_popup.cpp
  widgets/user_flow_popup.cpp
  widgets/edit_reviewers_popup.cpp
  misc/edit_reviewers.cpp
)

set(review_HEADERS
  app/review_app.h
  app/review_database.h
  app/review_client.h
  app/review_server.h
  data/activity_data.h
  data/comment_data.h
  data/user_data.h
  data/workspace_data.h
  data/project_data.h
  data/review_data.h
  data/user_settings_data.h
  elements/project_element.h
  elements/review_element.h
  elements/user_element.h
  elements/file_drop_element.h
  elements/inline_user_display_element.h
  elements/video_element.h
  elements/audio_element.h
  elements/file_element.h
  glad/include/glad/gl.h
  glad/include/KHR/khrplatform.h
  managers/project_manager.h
  managers/review_manager.h
  managers/workspace_manager.h
  managers/intro_manager.h
  subsystems/video_subsystem.h
  widgets/review_root_widget.h
  widgets/intro_widget.h
  widgets/workspace_widget.h
  widgets/create_comment_popup.h
  widgets/create_project_popup.h
  widgets/create_review_popup.h
  widgets/update_review_popup.h
  widgets/user_flow_popup.h
  widgets/edit_reviewers_popup.h
  misc/edit_reviewers.h
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Source Files" FILES ${review_SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Header Files" FILES ${review_HEADERS})

set(PCH_HEADER pch.h)
set(PCH_SOURCE pch.cpp)

include(ExternalProject)

function(get_mpv_win_dev name)
  ExternalProject_Add(${name}
    URL https://downloads.sourceforge.net/mpv-player-windows/mpv-dev-x86_64-20250713-git-bd21180.7z
    URL_HASH SHA256=e75982d90a1fa3620b194204420386f590b4e96f03dc9daf46b996fccce3549f
    DOWNLOAD_NO_PROGRESS ON
    UPDATE_COMMAND ""
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/include <BINARY_DIR>/include
            COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/libmpv.dll.a <BINARY_DIR>
            COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/libmpv-2.dll <BINARY_DIR>
            COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/libmpv-2.dll ${CMAKE_BINARY_DIR}
    )
  ExternalProject_Get_property(${name} BINARY_DIR)
  set(MPV_DEV_DIR ${BINARY_DIR})

  set(MPV_INCLUDE_DIRS ${MPV_DEV_DIR}/include PARENT_SCOPE)
  set(MPV_LIBRARY_DIRS ${MPV_DEV_DIR} PARENT_SCOPE)
  set(MPV_LIBRARIES mpv PARENT_SCOPE)
endfunction()

cmake_dependent_option(USE_MPV_WIN_BUILD "Use Prebuilt static mpv dll on Windows" ON "WIN32" OFF)

if(USE_MPV_WIN_BUILD)
  get_mpv_win_dev(mpv_dev)
else()
  pkg_search_module(MPV REQUIRED mpv>=0.33.0)
endif()

add_executable(review 
  ${PCH_SOURCE}
  ${review_SOURCES}
  ${review_HEADERS}
)

setup_tracy_profiler(review)

set_target_properties(review PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>)

cxx_20(review)

target_include_directories(review
    PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/glad/include
    ${MPV_INCLUDE_DIRS}
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

FetchContent_MakeAvailable(sqlitecpp)
FetchContent_MakeAvailable(magicenum)
FetchContent_MakeAvailable(httplib)

set(OPENSSL_ROOT_DIR ${PROJECT_SOURCE_DIR}/extern/openssl/x64)
set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL REQUIRED)

target_compile_definitions(review PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)

target_include_directories(review PRIVATE ${OPENSSL_ROOT_DIR}/include)

target_link_libraries(review
    PRIVATE
    SoundBakery::Gluten
    SQLiteCpp
    sqlite3
    magic_enum::magic_enum
    httplib::httplib
    OpenSSL::SSL
    OpenSSL::Crypto
)

if(USE_MPV_WIN_BUILD)
  add_dependencies(review mpv_dev)

  add_custom_command(TARGET review POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${MPV_LIBRARY_DIRS}/libmpv-2.dll
        $<TARGET_FILE_DIR:review>
  )
endif()

target_compile_definitions(review PRIVATE "MPV_CPLUGIN_DYNAMIC_SYM")

if(MSVC)
  target_compile_options(review PRIVATE /W4)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
else()
  target_compile_options(review PRIVATE -Wall -Wextra -Wpedantic)
endif()

install(TARGETS review DESTINATION apps)