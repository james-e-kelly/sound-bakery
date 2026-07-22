#include "system.h"

#include "sound_bakery/core/thread_domain.h"
#include "sound_bakery/error/result.h"
#include "sound_bakery/editor/project/project.h"
#include "sound_bakery/gameobject/gameobject.h"
#include "sound_bakery/node/bus/bus.h"
#include "sound_bakery/profiling/voice_tracker.h"
#include "sound_bakery/util/type_helper.h"
#include "spdlog/sinks/daily_file_sink.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "rpmalloc/rpmalloc.h"

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
    bool s_registeredReflection   = false;

    const std::string s_soundChefLoggerName("sound_chef");
    const std::string s_soundBakeryLoggerName("sound_bakery");

    auto miniaudio_log_callback(void* pUserData, ma_uint32 level, const char* pMessage) -> void
    {
        (void)pUserData;

        switch (level)
        {
            case MA_LOG_LEVEL_DEBUG:
                // Purposefully drop debug messages for now
                break;
            case MA_LOG_LEVEL_INFO:
                SBK_INFO("{}", pMessage);
                break;
            case MA_LOG_LEVEL_WARNING:
                SBK_WARN("{}", pMessage);
                break;
            case MA_LOG_LEVEL_ERROR:
                SBK_ERROR("{}", pMessage);
                break;
            default:
                break;
        }
    }
}  // namespace

system::system()
    : sc_system(), sbk::core::logger(s_soundBakeryLoggerName)
{
    concurrencpp::runtime_options runtimeOptions;
    runtimeOptions.thread_started_callback = [](std::string_view threadName) -> void { sbk::memory::thread_start(threadName); };
    runtimeOptions.thread_terminated_callback = [](std::string_view threadName) -> void { sbk::memory::thread_end(threadName); };

    m_threadRuntime             = std::make_unique<concurrencpp::runtime>(runtimeOptions);
    m_gameThreadExecuter        = std::make_shared<concurrencpp::manual_executor>();
    m_studioThreadExecuter      = std::make_shared<concurrencpp::manual_executor>();
    m_workerThread = std::make_shared<concurrencpp::worker_thread_executor>(runtimeOptions.thread_started_callback,
                                                                            runtimeOptions.thread_terminated_callback);

    const sbk_status initLogResult = sc_system_log_init(this, miniaudio_log_callback);
    sbk::log_error(initLogResult, "sc_system_log_init");
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
    m_studioThreadTimer.cancel();
    m_workerThread->shutdown();
    get_background_thread_executer()->shutdown();
    get_game_thread_executer()->shutdown();
    get_system_thread_executer()->shutdown();

    if (m_project)
    {
        m_project.reset();
    }

    remove_all();
    BOOST_ASSERT(get_objects_count() == 0);

    if (m_initSoundChef)
    {
        const sbk_status closeResult = sc_system_close(this);
        sbk::log_error(closeResult, "sc_system_close");
        m_initSoundChef = false;
    }

    spdlog::shutdown();
}

auto system::get() -> sbk::engine::system* { return s_system.load(std::memory_order_acquire); }

auto sbk::engine::system::get_operating_mode() -> operating_mode
{
    if (m_project)
    {
        return operating_mode::editor;
    }

    if (get_objects_count())
    {
        return operating_mode::runtime;
    }

    return operating_mode::unkown;
}

auto system::create() -> sbk::result<void>
{
    if (s_system.load(std::memory_order_acquire) == nullptr)
    {
        void* const systemMemory = sbk::memory::malloc(sizeof(system), SB_OBJECT_CATEGORY::SB_CATEGORY_SYSTEM);

        if (systemMemory == nullptr)
        {
            return sbk::make_error(SBK_ERR_OUT_OF_MEMORY, "Could not create the system object");
        }
        s_system.store(::new (systemMemory) system(), std::memory_order_release);
    }

    SBK_CHECK(s_system.load(std::memory_order_acquire) != nullptr, SBK_ERR_OUT_OF_MEMORY);
    return sbk::ok();
}

auto system::create(const std::filesystem::path& logFile) -> sbk::result<void>
{
    if (s_system.load(std::memory_order_acquire) == nullptr)
    {
        void* const systemMemory = sbk::memory::malloc(sizeof(system), SB_OBJECT_CATEGORY::SB_CATEGORY_SYSTEM);

        if (systemMemory == nullptr)
        {
            return sbk::make_error(SBK_ERR_NULL, "Could not create the system object");
        }
        s_system.store(::new (systemMemory) system(logFile), std::memory_order_release);
    }

    SBK_CHECK(s_system.load(std::memory_order_acquire) != nullptr, SBK_ERR_OUT_OF_MEMORY);
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
    sbk_system_config configCopy = config;
    configCopy.soundChefConfig.allocationCallbacks.pUserData = this;
    configCopy.soundChefConfig.allocationCallbacks.onMalloc = ma_malloc;
    configCopy.soundChefConfig.allocationCallbacks.onRealloc = ma_realloc;
    configCopy.soundChefConfig.allocationCallbacks.onFree = ma_free;

    if (configCopy.logToConsole)
    {
        add_console_sink();
    }
    
    SBK_INFO("Initializing Sound Bakery");

    SBK_TRY_C(sc_system_init(this, &configCopy.soundChefConfig));  //< Logs and forwards the error if init fails.
    m_initSoundChef = true;

    if (!s_registeredReflection)
    {
        sbk::reflection::register_reflection_types();
        s_registeredReflection = true;
    }

    SBK_TRY(auto listener, create_database_object<sbk::engine::game_object>());
    listener->set_object_name("Listener");
    listener->set_editor_hidden(true);
    m_listenerGameObject = listener;

#if SBK_CONFIG_ENABLE_PROFILING
    if (config.enableProfiling)
    {
        m_voiceTracker = std::make_unique<profiling::voice_tracker>();
    }
#endif

    // The config chooses the threading mode; mirror it to the thread-domain
    // checks so single-threaded (editor) access outside a drain scope is allowed.
    sbk::core::set_single_threaded_mode(config.singleThreadedUpdate);

    if (!config.singleThreadedUpdate)
    {
        m_studioThreadTimer = m_threadRuntime->timer_queue()->make_timer(0ms, 20ms, m_workerThread,
            [this]
            {
                update_async();
            });
    }

    return sbk::ok();
}

auto system::update() -> sbk::result<void>
{
    FrameMarkStart(profiling_strings::s_updateName);
    ZoneScoped;

    // If we're not running the update_async on a different thread, run it during update
    // This operating mode lets consuming applications (the editor) access database data safely
    if (!m_studioThreadTimer)
    {
        update_async();
    }

    const sbk::core::scoped_thread_domain gameDomain(sbk::core::thread_domain::game);

    m_gameThreadExecuter->loop(m_gameThreadExecuter->size());

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
    TracyPlot(profiling_strings::s_totalMemory,(int64_t) stats.mapped_total);

    TracyPlotConfig(profiling_strings::s_currentMemory, tracy::PlotFormatType::Memory, true, true, 0);
    TracyPlot(profiling_strings::s_currentMemory, (int64_t)stats.mapped);
#endif

    FrameMarkEnd(profiling_strings::s_updateName);

    return sbk::ok();
}

auto sbk::engine::system::update_async() -> void
{
    ZoneScoped;

    // The whole drain window is the studio domain - the executor tasks and
    // the game object updates below both count as studio-owned work.
    const sbk::core::scoped_thread_domain studioDomain(sbk::core::thread_domain::studio);

    m_studioThreadExecuter->loop(m_studioThreadExecuter->size());

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

auto system::get_current_object_owner() -> sbk::core::object_owner* { return m_project.get(); }

auto system::open_project(const std::filesystem::path& projectFile, sbk::core::sbk_log_callback_proc logCallback) -> sbk::result<void>
{
    destroy();

    const sbk::editor::project_configuration tempProject = sbk::editor::project_configuration(projectFile);

    SBK_TRYV(create(tempProject.log_folder() / (std::string(tempProject.project_name()) + ".txt")));

    // create() above succeeded, so s_system is now set.
    system* const sys = s_system.load(std::memory_order_acquire);

    if (logCallback)
    {
        sys->add_external_log(logCallback);
    }

    const std::string pluginFolder      = tempProject.plugin_folder().string();
    const sbk_system_config systemConfig = sbk_system_config_init(pluginFolder.c_str());

    SBK_TRYV(sys->init(systemConfig));

    sys->m_project = std::make_unique<sbk::editor::project>();

    if (sbk::result<void> opened = sys->m_project->open_project(projectFile); !opened.has_value())
    {
        sys->m_project.reset();
        return tl::make_unexpected(std::move(opened).error());  //< Already logged at origin; forward it.
    }

    return sbk::ok();
}

auto sbk::engine::system::create_project(const std::filesystem::directory_entry& projectDirectory, std::string_view projectName) -> sbk::result<void>
{
    const sbk::editor::project_configuration projectConfig(projectDirectory, projectName);

    SBK_TRYV(open_project(projectConfig.project_file(), nullptr));

    // open_project() above succeeded, so s_system is now set.
    system* const sys = s_system.load(std::memory_order_acquire);
    SBK_CHECK_MSG(sys->m_project != nullptr, SBK_ERR_BAKERY, "System's project variable was null");

    SBK_TRY(auto masterBus, sys->m_project->create_database_object<sbk::engine::bus>());
    masterBus->set_object_name("Master Bus");
    masterBus->set_master_bus(true);

    return sys->m_project->save_project();
}

auto system::get_project() const -> sbk::editor::project*
{
    return m_project.get();
}

auto system::get_voice_tracker() const -> sbk::engine::profiling::voice_tracker*
{
    return m_voiceTracker.get();
}

auto sbk::engine::system::get_game_thread_executer() const -> std::shared_ptr<concurrencpp::manual_executor>
{
    return m_gameThreadExecuter;
}

auto sbk::engine::system::get_system_thread_executer() const -> std::shared_ptr<concurrencpp::manual_executor>
{
    return m_studioThreadExecuter;
}

auto sbk::engine::system::get_background_thread_executer() const -> std::shared_ptr<concurrencpp::thread_pool_executor>
{
    return m_threadRuntime ? m_threadRuntime->background_executor() : std::shared_ptr<concurrencpp::thread_pool_executor>{};
}

auto sbk::engine::system::get_listener_game_object() const -> std::shared_ptr<sbk::engine::game_object>
{
    return m_listenerGameObject.lock();
}

auto sbk::engine::system::get_master_bus() const -> std::shared_ptr<sbk::engine::bus> 
{ 
    return m_masterBus.lock(); 
}

auto sbk::engine::system::set_master_bus(const std::shared_ptr<sbk::engine::bus>& masterBus) -> void
{
    /// @todo What do we need to do about the old master bus, if there is one?
    /// Is it safe to destroy any original master bus - like a serialized bus is loaded and overrides the default one?
    m_masterBus = masterBus;
}
