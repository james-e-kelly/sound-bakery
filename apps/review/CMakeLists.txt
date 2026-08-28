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
  elements/left_panel_element.cpp
  elements/review_element.cpp
  elements/toolbar_element.cpp
  elements/project_element.cpp
  elements/list_element.cpp
  elements/video_element.cpp
  elements/audio_element.cpp
  managers/project_manager.cpp
  managers/review_manager.cpp
  managers/workspace_manager.cpp
  managers/intro_manager.cpp
  widgets/review_root_widget.cpp
  widgets/intro_widget.cpp
  widgets/workspace_widget.cpp
  widgets/create_comment_popup.cpp
  widgets/create_project_popup.cpp
  widgets/create_review_popup.cpp
  widgets/settings_popup.cpp
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
  elements/audio_element.h
  elements/left_panel_element.h
  elements/file_drop_element.h
  elements/inline_user_display_element.h
  elements/list_element.h
  elements/toolbar_element.h
  elements/project_element.h
  elements/review_element.h
  elements/user_element.h
  elements/video_element.h
  managers/project_manager.h
  managers/review_manager.h
  managers/workspace_manager.h
  managers/intro_manager.h
  widgets/review_root_widget.h
  widgets/intro_widget.h
  widgets/workspace_widget.h
  widgets/create_comment_popup.h
  widgets/create_project_popup.h
  widgets/create_review_popup.h
  widgets/settings_popup.h
  widgets/update_review_popup.h
  widgets/user_flow_popup.h
  widgets/edit_reviewers_popup.h
  misc/edit_reviewers.h
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Source Files" FILES ${review_SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Header Files" FILES ${review_HEADERS})

set(PCH_HEADER pch.h)
set(PCH_SOURCE pch.cpp)

# mpv is fetched by gluten (build_mpv.cmake), which video_subsystem now lives in.
# review still needs libmpv-2.dll copied next to its own executable at runtime.

add_executable(review
  ${PCH_SOURCE}
  ${review_SOURCES}
  ${review_HEADERS}
)

setup_tracy_profiler(review)

cxx_20(review)

target_include_directories(review
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
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

include(build_review)
setup_review_dependencies(review)

target_link_libraries(review
    PRIVATE
    SoundBakery::Gluten
)

if(USE_MPV_WIN_BUILD)
  add_dependencies(review mpv_dev)

  add_custom_command(TARGET review POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${MPV_LIBRARY_DIRS}/libmpv-2.dll
        $<TARGET_FILE_DIR:review>
  )
endif()

target_compile_options(review PRIVATE ${PROJECT_WARNING_FLAGS})

set(CMAKE_FOLDER apps)

sbk_add_format_target(NAME review
    SOURCES ${review_SOURCES} ${review_HEADERS} ${PCH_HEADER} ${PCH_SOURCE})

if (SOUND_BAKERY_INSTALL)
sbk_install(TARGETS review DESTINATION apps/${SBK_INSTALL_CONFIG_SUBDIR})
endif()