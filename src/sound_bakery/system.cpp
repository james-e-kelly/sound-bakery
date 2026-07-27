#include "system.h"

#include "sound_bakery/core/thread_domain.h"
#include "sound_bakery/editor/project/project.h"
#include "sound_bakery/error/result.h"
#include "sound_bakery/gameobject/gameobject.h"
#include "sound_bakery/node/bus/bus.h"
#include "sound_bakery/profiling/voice_tracker.h"
#include "sound_bakery/runtime/runtime.h"
#include "sound_bakery/task/command_queue.h"
#include "sound_bakery/task/manual_executor.h"
#include "sound_bakery/task/task.h"
#include "sound_bakery/task/thread_executor.h"
#include "sound_bakery/util/type_helper.h"

#include "rpmalloc/rpmalloc.h"
#include "spdlog/sinks/daily_file_sink.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

using namespace sbk::engine;
using namespace std::chrono_literals;

namespace profiling_strings
{
    static const char* const s_updateName           = "SoundBakeryUpdate";
    static const char* const s_gameObjectPlotName   = "Number Of Game Objects";
    static const char* const s_voicePlotName        = "Number Of Voices";
    static const char* const s_nodeInstancePlotName = "Number Of Node Instances";
    static const char* const s_totalMemory          = "Total Memory";
    static const char* const s_currentMemory        = "Current Memory";
}  // namespace profiling_strings

auto ma_malloc(std::size_t size, void* userData) -> void*
{
    return sbk::memory::malloc(size, SB_CATEGORY_UNKNOWN);
}

auto ma_realloc(void* pointer, std::size_t size, void* userData) -> void*
{
    return sbk::memory::realloc(pointer, size);
}

auto ma_free(void* pointer, void* userData) -> void
{
    sbk::memory::free(pointer, SB_CATEGORY_UNKNOWN);
}

namespace
{
    // The single global system instance.
    //
    // Threading model (FMOD-style): create() and destroy() are lifecycle operations the
    // caller must serialize against each other and against all other API use - in practice,
    // create once at startup and destroy once at shutdown, with no other thread calling into
    // Sound Bakery during either. While the system is alive, get() and the runtime operations
    // it feeds (logging in particular) are safe to call from any thread.
    //
    // The atomic makes the steady-state read in get() formally race-free for those concurrent
    // callers; it does NOT by itself make it safe to destroy the system while another thread is
    // still using the returned pointer - that safety comes from the create/destroy contract
    // above, not from the atomic.
    std::atomic<sbk::engine::system*> s_system{nullptr};
    bool s_registeredReflection = false;

    const std::string s_soundChefLoggerName("sound_chef");
    const std::string s_soundBakeryLoggerName("sound_bakery");

    /**
     * @brief Device audio callback. Customised to add profiling.
     */
    void sbk_audio_data_callback(ma_device* device, void* output, const void* input, ma_uint32 frameCount)
    {
        (void)input;

#ifdef TRACY_ENABLE
        thread_local bool nameSet = false;
        if (!nameSet)
        {
            tracy::SetThreadName("Sound Bakery Audio");
            nameSet = true;
        }

        ZoneScopedN("Audio Render");
#endif
        (void)ma_engine_read_pcm_frames(static_cast<ma_engine*>(device->pUserData), output, frameCount, nullptr);
    }

    /**
     * @brief Allocate memory for the system and store it in s_system.
     * 
     * The object will not be constructed.
     */
    auto allocate_system_memory() -> sbk::result<>
    {
        if (s_system.load(std::memory_order_acquire) == nullptr)
        {
            void* const systemMemory = sbk::memory::malloc(sizeof(sbk::engine::system), SB_OBJECT_CATEGORY::SB_CATEGORY_SYSTEM);

            if (systemMemory == nullptr)
            {
                return sbk::make_error(SBK_ERR_NULL, "Could not create the system object");
            }
            s_system.store(static_cast<sbk::engine::system*>(systemMemory), std::memory_order_release);
        }
        else
        {
            return sbk::make_error(SBK_ERR_BAKERY_OBJECT_EXISTS, "System memory was already allocated. Should not call the constructor on it");
        }
        return sbk::ok();
    }
}  // namespace

system::system()
    : sc_system(), sbk::core::logger(s_soundBakeryLoggerName)
{
}

system::system(const std::filesystem::path& logFile)
    : system()
{
    add_file_sink(logFile.string());
}

system::system(sbk::core::sbk_log_callback_proc logCallback)
    : system()
{
    add_external_log(logCallback);
}

system::~system()
{
    if (m_project)
    {
        m_project.reset();
    }

    (void)m_systemExecutor->drain();
    (void)m_gameExecutor->drain();
    (void)m_workerThread->drain();
    if (m_systemThread)
    {
        (void)m_systemThread->drain();
    }

    if (m_runtime)
    {
        m_runtime.reset();
    }

    spdlog::shutdown();
}

auto system::get() -> sbk::engine::system* { return s_system.load(std::memory_order_acquire); }

auto system::create() -> sbk::result<void>
{
    SBK_TRYV(allocate_system_memory());
    ::new (s_system.load(std::memory_order_relaxed)) sbk::engine::system();
    return sbk::ok();
}

auto system::create(const std::filesystem::path& logFile) -> sbk::result<void>
{
    SBK_TRYV(allocate_system_memory());
    ::new (s_system.load(std::memory_order_relaxed)) sbk::engine::system(logFile);
    return sbk::ok();
}

auto system::create(sbk::core::sbk_log_callback_proc logCallback) -> sbk::result<void>
{
    SBK_TRYV(allocate_system_memory());
    ::new (s_system.load(std::memory_order_relaxed)) sbk::engine::system(logCallback);
    return sbk::ok();
}

auto system::destroy() -> void
{
    system* const sys = s_system.load(std::memory_order_acquire);
    if (sys != nullptr)
    {
        SBK_INFO("Closing and destroying Sound Bakery");

        sys->~system();
        sbk::memory::free(sys, SB_CATEGORY_SYSTEM);
        s_system.store(nullptr, std::memory_order_release);
    }
}

auto system::init(const sbk_system_config& config) -> sbk::result<void>
{
    SBK_TRY(m_runtime, create_owned<sbk::engine::runtime>(SB_CATEGORY_SYSTEM));
     
    sbk_system_config configCopy                             = config;
    configCopy.soundChefConfig.allocationCallbacks.pUserData = this;
    configCopy.soundChefConfig.allocationCallbacks.onMalloc  = ma_malloc;
    configCopy.soundChefConfig.allocationCallbacks.onRealloc = ma_realloc;
    configCopy.soundChefConfig.allocationCallbacks.onFree    = ma_free;

    // Override the engine device's data callback so the mix runs (and is profiled) inside Sound
    // Bakery. miniaudio still creates and owns the device and its thread; only the callback body is
    // ours. See sbk_audio_data_callback above.
    configCopy.soundChefConfig.dataCallback = &sbk_audio_data_callback;

    if (configCopy.logToConsole)
    {
        add_console_sink();
    }

    SBK_INFO("Initializing Sound Bakery");

    if (!s_registeredReflection)
    {
        sbk::reflection::register_reflection_types();
        s_registeredReflection = true;
    }

    SBK_TRYV(m_runtime->init(configCopy.soundChefConfig));

#if SBK_CONFIG_ENABLE_PROFILING
    if (config.enableProfiling)
    {
        SBK_TRY(m_voiceTracker, create_owned<sbk::engine::profiling::voice_tracker>(SB_CATEGORY_SYSTEM));
    }
#endif

    sbk::core::set_single_threaded_mode(config.singleThreadedUpdate);

    m_gameExecutor          = std::make_shared<sbk::manual_executor>("Game Thread");
    m_workerThread          = std::make_shared<sbk::thread_executor>("Worker Thread");
    auto studioCommandQueue = std::make_shared<sbk::command_queue>("System Command Queue");

    if (config.singleThreadedUpdate)
    {
        studioCommandQueue->m_target = m_gameExecutor.get();
    }
    else
    {
        m_systemThread               = std::make_shared<sbk::thread_executor>("System Thread");
        studioCommandQueue->m_target = m_systemThread.get();
    }

    m_systemExecutor = studioCommandQueue;

    return sbk::ok();
}

auto system::update() -> sbk::result<void>
{
    FrameMarkStart(profiling_strings::s_updateName);
    ZoneScoped;

    SBK_TRYV(m_systemExecutor->post_work([this]()
                                         { update_async(); }));
    SBK_TRYV(m_systemExecutor->flush());

    const sbk::core::scoped_thread_domain gameDomain(sbk::core::thread_domain::game);
    SBK_TRYV(m_gameExecutor->drain());

#if SBK_CONFIG_ENABLE_PROFILING
    if (m_voiceTracker)
    {
        m_voiceTracker->update(this);
    }

    TracyPlotConfig(profiling_strings::s_gameObjectPlotName, tracy::PlotFormatType::Number, true, false, 0);
    TracyPlotConfig(profiling_strings::s_nodeInstancePlotName, tracy::PlotFormatType::Number, true, false, 0);
    TracyPlotConfig(profiling_strings::s_voicePlotName, tracy::PlotFormatType::Number, true, false, 0);

    TracyPlot(profiling_strings::s_gameObjectPlotName, (int64_t)get_objects_of_type(sbk::engine::game_object::type()).size());
    TracyPlot(profiling_strings::s_voicePlotName, (int64_t)get_objects_of_type(sbk::engine::voice::type()).size());
    TracyPlot(profiling_strings::s_nodeInstancePlotName, (int64_t)get_objects_of_type(sbk::engine::node_instance::type()).size());

    rpmalloc_global_statistics_t stats;
    rpmalloc_global_statistics(&stats);

    TracyPlotConfig(profiling_strings::s_totalMemory, tracy::PlotFormatType::Memory, true, true, 0);
    TracyPlot(profiling_strings::s_totalMemory, (int64_t)stats.mapped_total);

    TracyPlotConfig(profiling_strings::s_currentMemory, tracy::PlotFormatType::Memory, true, true, 0);
    TracyPlot(profiling_strings::s_currentMemory, (int64_t)stats.mapped);
#endif

    FrameMarkEnd(profiling_strings::s_updateName);

    return sbk::ok();
}

auto sbk::engine::system::update_async() -> void
{
    ZoneScoped;

    for (const auto& object : get_objects_of_type(sbk::engine::game_object::type()))
    {
        if (object)
        {
            if (sbk::engine::game_object* const gameObject = object->try_convert_object<sbk::engine::game_object>())
            {
                gameObject->update();
            }
        }
    }
}

auto system::get_current_object_owner() -> sbk::core::object_owner* 
{ 
    if (m_project)
    {
        return m_project.get();
    }

    return m_runtime.get();
}

auto system::get_project() const -> sbk::editor::project*
{
    return m_project.get();
}

auto system::get_runtime() const -> sbk::engine::runtime*
{
    return m_runtime.get();
}

auto system::get_voice_tracker() const -> sbk::engine::profiling::voice_tracker*
{
    return m_voiceTracker.get();
}

auto sbk::engine::system::get_game_executer() const -> std::shared_ptr<sbk::executor>
{
    return m_gameExecutor;
}

auto sbk::engine::system::get_system_executer() const -> std::shared_ptr<sbk::executor>
{
    return m_systemExecutor;
}

auto sbk::engine::system::get_worker_executer() const -> std::shared_ptr<sbk::executor>
{
    return m_workerThread;
}

auto sbk::engine::system::create_project() -> sbk::result<sbk::editor::project*>
{
    SBK_TRY(m_project, create_owned<sbk::editor::project>(SB_CATEGORY_SYSTEM));
    return m_project.get();
}