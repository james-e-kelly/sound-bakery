set(CMAKE_FOLDER extern)
FetchContent_MakeAvailable(tracy)

CPMAddPackage(
    NAME PPQSort
    GITHUB_REPOSITORY foxtran/PPQSort
    GIT_TAG master
    EXCLUDE_FROM_ALL TRUE
)

CPMAddPackage(
    NAME capstone
    GITHUB_REPOSITORY capstone-engine/capstone
    GIT_TAG next
    OPTIONS
        "CAPSTONE_X86_ATT_DISABLE ON"
        "CAPSTONE_ALPHA_SUPPORT OFF"
        "CAPSTONE_HPPA_SUPPORT OFF"
        "CAPSTONE_LOONGARCH_SUPPORT OFF"
        "CAPSTONE_M680X_SUPPORT OFF"
        "CAPSTONE_M68K_SUPPORT OFF"
        "CAPSTONE_MIPS_SUPPORT OFF"
        "CAPSTONE_MOS65XX_SUPPORT OFF"
        "CAPSTONE_PPC_SUPPORT OFF"
        "CAPSTONE_SPARC_SUPPORT OFF"
        "CAPSTONE_SYSTEMZ_SUPPORT OFF"
        "CAPSTONE_XCORE_SUPPORT OFF"
        "CAPSTONE_TRICORE_SUPPORT OFF"
        "CAPSTONE_TMS320C64X_SUPPORT OFF"
        "CAPSTONE_M680X_SUPPORT OFF"
        "CAPSTONE_EVM_SUPPORT OFF"
        "CAPSTONE_WASM_SUPPORT OFF"
        "CAPSTONE_BPF_SUPPORT OFF"
        "CAPSTONE_RISCV_SUPPORT OFF"
        "CAPSTONE_SH_SUPPORT OFF"
        "CAPSTONE_XTENSA_SUPPORT OFF"
        "CAPSTONE_BUILD_MACOS_THIN ON"
    EXCLUDE_FROM_ALL TRUE
)

set(ZSTD_DIR "${tracy_SOURCE_DIR}/zstd")
set(TRACY_COMMON_DIR ${tracy_SOURCE_DIR}/public/common)
set(TRACY_SERVER_DIR ${tracy_SOURCE_DIR}/server)

set(ZSTD_SOURCES
    decompress/zstd_ddict.c
    decompress/zstd_decompress_block.c
    decompress/huf_decompress.c
    decompress/zstd_decompress.c
    common/zstd_common.c
    common/error_private.c
    common/xxhash.c
    common/entropy_common.c
    common/debug.c
    common/threading.c
    common/pool.c
    common/fse_decompress.c
    compress/zstd_ldm.c
    compress/zstd_compress_superblock.c
    compress/zstd_opt.c
    compress/zstd_compress_sequences.c
    compress/fse_compress.c
    compress/zstd_double_fast.c
    compress/zstd_compress.c
    compress/zstd_compress_literals.c
    compress/hist.c
    compress/zstdmt_compress.c
    compress/zstd_lazy.c
    compress/huf_compress.c
    compress/zstd_fast.c
    dictBuilder/zdict.c
    dictBuilder/cover.c
    dictBuilder/divsufsort.c
    dictBuilder/fastcover.c
)

set(TRACY_COMMON_SOURCES
    tracy_lz4.cpp
    tracy_lz4hc.cpp
    TracySocket.cpp
    TracyStackFrames.cpp
    TracySystem.cpp
)

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
list(TRANSFORM TRACY_COMMON_SOURCES PREPEND "${TRACY_COMMON_DIR}/")
list(TRANSFORM TRACY_SERVER_SOURCES PREPEND "${TRACY_SERVER_DIR}/")
list(TRANSFORM ZSTD_SOURCES PREPEND "${ZSTD_DIR}/")

set_property(SOURCE ${ZSTD_DIR}/decompress/huf_decompress_amd64.S APPEND PROPERTY COMPILE_OPTIONS "-x" "assembler-with-cpp")

add_library(TracyZstd STATIC ${ZSTD_SOURCES})
target_include_directories(TracyZstd PUBLIC ${ZSTD_DIR})
target_compile_definitions(TracyZstd PRIVATE ZSTD_DISABLE_ASM)
c_17(TracyZstd)
cxx_20(TracyZstd)

add_library(sbk_tracy_server STATIC ${TRACY_COMMON_SOURCES} ${TRACY_SERVER_SOURCES})
target_include_directories(sbk_tracy_server PUBLIC ${TRACY_COMMON_DIR} ${TRACY_SERVER_DIR} ${capstone_SOURCE_DIR}/include/capstone)
target_link_libraries(sbk_tracy_server PUBLIC capstone_static TracyZstd PPQSort::PPQSort)
c_17(sbk_tracy_server)
cxx_20(sbk_tracy_server)

if(SELF_PROFILE)
    add_definitions(-DTRACY_ENABLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O3 -fno-omit-frame-pointer")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O3 -fno-omit-frame-pointer")
    set(PROFILER_FILES ${PROFILER_FILES} ${tracy_SOURCE_DIR}/public/TracyClient.cpp)
endif()

add_library(sbk_tracy_profiler STATIC ${PROFILER_FILES} ${COMMON_FILES} ${SERVER_FILES} ${tracy_SOURCE_DIR}/profiler/src/ini.c)
target_link_libraries(sbk_tracy_profiler PUBLIC sbk_tracy_server imgui nfd)
target_include_directories(sbk_tracy_profiler PUBLIC ${tracy_SOURCE_DIR}/profiler/src/profiler)
target_compile_definitions(sbk_tracy_profiler PUBLIC TRACY_NO_ROOT_WINDOW)
add_library(sbk::tracy_profiler ALIAS sbk_tracy_profiler)
c_17(sbk_tracy_profiler)
cxx_20(sbk_tracy_profiler)