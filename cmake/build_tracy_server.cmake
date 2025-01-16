set(CMAKE_FOLDER extern)
FetchContent_MakeAvailable(tracy)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(${tracy_SOURCE_DIR}/profiler ${tracy_BINARY_DIR}/profiler EXCLUDE_FROM_ALL)

set(TRACY_COMMON_DIR ${tracy_SOURCE_DIR}/public/common)

set(TRACY_COMMON_SOURCES
    tracy_lz4.cpp
    tracy_lz4hc.cpp
    TracySocket.cpp
    TracyStackFrames.cpp
    TracySystem.cpp
)

list(TRANSFORM TRACY_COMMON_SOURCES PREPEND "${TRACY_COMMON_DIR}/")

set(TRACY_SERVER_DIR ${tracy_SOURCE_DIR}/server)

set(TRACY_SERVER_SOURCES
    TracyMemory.cpp
    TracyMmap.cpp
    TracyPrint.cpp
    TracySysUtil.cpp
    TracyTaskDispatch.cpp
    TracyTextureCompression.cpp
    TracyThreadCompress.cpp
    TracyWorker.cpp
)

list(TRANSFORM TRACY_SERVER_SOURCES PREPEND "${TRACY_SERVER_DIR}/")

add_library(sbk_tracy_server STATIC EXCLUDE_FROM_ALL ${TRACY_COMMON_SOURCES} ${TRACY_SERVER_SOURCES})
target_include_directories(sbk_tracy_server PUBLIC ${TRACY_COMMON_DIR} ${TRACY_SERVER_DIR})
target_link_libraries(sbk_tracy_server PUBLIC TracyCapstone TracyZstd PPQSort::PPQSort)

set(SERVER_FILES
    TracyAchievementData.cpp
    TracyAchievements.cpp
    TracyBadVersion.cpp
    TracyColor.cpp
    TracyEventDebug.cpp
    TracyFileselector.cpp
    TracyFilesystem.cpp
    TracyMicroArchitecture.cpp
    TracyMouse.cpp
    TracyProtoHistory.cpp
    TracySourceContents.cpp
    TracySourceTokenizer.cpp
    TracySourceView.cpp
    TracyStorage.cpp
    TracyTexture.cpp
    TracyImGui.cpp
    TracyTimelineController.cpp
    TracyTimelineItem.cpp
    TracyTimelineItemCpuData.cpp
    TracyTimelineItemGpu.cpp
    TracyTimelineItemPlot.cpp
    TracyTimelineItemThread.cpp
    TracyUserData.cpp
    TracyUtility.cpp
    TracyView.cpp
    TracyView_Annotations.cpp
    TracyView_Callstack.cpp
    TracyView_Compare.cpp
    TracyView_ConnectionState.cpp
    TracyView_ContextSwitch.cpp
    TracyView_CpuData.cpp
    TracyView_FindZone.cpp
    TracyView_FlameGraph.cpp
    TracyView_FrameOverview.cpp
    TracyView_FrameTimeline.cpp
    TracyView_FrameTree.cpp
    TracyView_GpuTimeline.cpp
    TracyView_Locks.cpp
    TracyView_Memory.cpp
    TracyView_Messages.cpp
    TracyView_Navigation.cpp
    TracyView_NotificationArea.cpp
    TracyView_Options.cpp
    TracyView_Playback.cpp
    TracyView_Plots.cpp
    TracyView_Ranges.cpp
    TracyView_Samples.cpp
    TracyView_Statistics.cpp
    TracyView_Timeline.cpp
    TracyView_TraceInfo.cpp
    TracyView_Utility.cpp
    TracyView_ZoneInfo.cpp
    TracyView_ZoneTimeline.cpp
    TracyWeb.cpp
)
list(TRANSFORM SERVER_FILES PREPEND ${tracy_SOURCE_DIR}/profiler/src/profiler/)

if(SELF_PROFILE)
    add_definitions(-DTRACY_ENABLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O3 -fno-omit-frame-pointer")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O3 -fno-omit-frame-pointer")
    set(PROFILER_FILES ${PROFILER_FILES} ${tracy_SOURCE_DIR}/public/TracyClient.cpp)
endif()

add_library(sbk_tracy_profiler STATIC ${PROFILER_FILES} ${COMMON_FILES} ${SERVER_FILES} ${tracy_SOURCE_DIR}/profiler/src/ini.c)
add_library(sbk::tracy_profiler ALIAS sbk_tracy_profiler)
target_link_libraries(sbk_tracy_profiler PUBLIC sbk_tracy_server imgui nfd)
target_include_directories(sbk_tracy_profiler PUBLIC ${tracy_SOURCE_DIR}/profiler/src/profiler)
target_compile_definitions(sbk_tracy_profiler PUBLIC TRACY_NO_ROOT_WINDOW)