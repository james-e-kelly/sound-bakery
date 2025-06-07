#include "system.h"

#include "sound_bakery/editor/project/project.h"
#include "sound_bakery/gameobject/gameobject.h"
#include "sound_bakery/node/bus/bus.h"
#include "sound_bakery/profiling/voice_tracker.h"
#include "sound_bakery/reflection/reflection.h"
#include "sound_bakery/serialization/serializer.h"
#include "sound_bakery/util/type_helper.h"
#include "spdlog/sinks/daily_file_sink.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "rpmalloc/rpmalloc.h"

using namespace sbk::engine;
using namespace std::chrono_literals;

namespace profiling_strings
{
    constexpr const char* const g_updateName           = "SoundBakeryUpdate";
    constexpr const char* const g_gameObjectPlotName   = "Number Of Game Objects";
    constexpr const char* const g_voicePlotName        = "Number Of Voices";
    constexpr const char* const g_nodeInstancePlotName = "Number Of Node Instances";
    constexpr const char* const g_totalMemory          = "Total Memory";
    constexpr const char* const g_currentMemory        = "Current Memory";
}  // namespace profiling_strings

namespace
{
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

    std::unique_ptr<sbk::engine::system> g_system;
    bool g_registeredReflection   = false;

    constexpr const char* g_soundChefLoggerName = "LogSoundChef";

    void miniaudio_log_callback(void* pUserData, ma_uint32 level, const char* pMessage)
    {
        (void)pUserData;

        auto soundChefLogger = spdlog::get(g_soundChefLoggerName);

        if (!soundChefLogger)
        {
            return;
        }

        switch (level)
        {
            case MA_LOG_LEVEL_DEBUG:
                soundChefLogger->debug("{}", pMessage);
                break;
            case MA_LOG_LEVEL_INFO:
                soundChefLogger->info("{}", pMessage);
                break;
            case MA_LOG_LEVEL_WARNING:
                soundChefLogger->warn("{}", pMessage);
                break;
            case MA_LOG_LEVEL_ERROR:
                soundChefLogger->error("{}", pMessage);
                break;
            default:
                break;
        }
    }
}  // namespace

system::system()
    : sc_system()
{
    concurrencpp::runtime_options runtimeOptions;
    runtimeOptions.thread_started_callback = [](std::string_view threadName) -> void { sbk::memory::thread_start(threadName); };
    runtimeOptions.thread_terminated_callback = [](std::string_view threadName) -> void { sbk::memory::thread_end(threadName); };

    m_threadRuntime             = std::make_unique<concurrencpp::runtime>(runtimeOptions);
    m_gameThreadExecuter        = std::make_shared<concurrencpp::manual_executor>();
    m_studioThreadExecuter      = std::make_shared<concurrencpp::manual_executor>();
    m_workerThread = std::make_shared<concurrencpp::worker_thread_executor>(runtimeOptions.thread_started_callback,
                                                                            runtimeOptions.thread_terminated_callback);

    const sbk_result initLogResult = sc_system_log_init(this, miniaudio_log_callback);
    BOOST_ASSERT(initLogResult == SBK_SUCCESS);
}

system::~system()
{
    
}

auto sbk::engine::system::operator new(std::size_t size) -> void* 
{
    return sbk::memory::malloc(size, SB_CATEGORY_SYSTEM);
}

auto sbk::engine::system::operator delete(void* ptr) -> void
{
    sbk::memory::free(ptr, SB_CATEGORY_SYSTEM);
}

sbk::engine::system* system::get() { return g_system.get(); }

auto sbk::engine::system::get_operating_mode() -> operating_mode
{
    if (g_system)
    {
        if (g_system->m_project)
        {
            return operating_mode::editor;
        }

        if (g_system->get_referenced_objects_size())
        {
            return operating_mode::runtime;
        }
    }

    return operating_mode::unkown;
}

auto system::create() -> sbk_result
{
    if (g_system == nullptr)
    {
        g_system = std::make_unique<system>();
    }

    return g_system ? SBK_SUCCESS : SBK_ERR_OUT_OF_MEMORY;
}

void system::destroy()
{
    if (g_system != nullptr)
    {
        g_system->close_system();
        g_system.reset();
    }
}

auto system::close_system() -> void
{
    SBK_INFO("Closing Sound Bakery");

    m_studioThreadTimer.cancel();

    if (m_project)
    {
        m_project.reset();
    }

    if (m_listenerGameObject)
    {
        m_listenerGameObject.reset();
    }

    if (m_masterBus)
    {
        m_masterBus.reset();
    }

    remove_all();
    BOOST_ASSERT(get_referenced_objects_size().get() == 0);
    
    sc_system_close(this);

    // Close threads
    get_game_thread_executor()->shutdown();
    get_system_thread_executor()->shutdown();

    get_thread_pool_executor()->shutdown();
    get_background_thread_executor()->shutdown();

    m_workerThread->shutdown();

    spdlog::shutdown();
}

auto system::init(const sbk_system_config& config) -> sbk_result
{
    if (g_system == nullptr)
    {
        return SBK_ERR_BAKERY_UNINITIALIZED;
    }

    SBK_INFO("Initializing Sound Bakery");

    sbk_system_config configCopy = config;
    configCopy.soundChefConfig.allocationCallbacks.pUserData = g_system.get();
    configCopy.soundChefConfig.allocationCallbacks.onMalloc = ma_malloc;
    configCopy.soundChefConfig.allocationCallbacks.onRealloc = ma_realloc;
    configCopy.soundChefConfig.allocationCallbacks.onFree = ma_free;

    const sbk_result result = sc_system_init(g_system.get(), &configCopy.soundChefConfig);
    BOOST_ASSERT(result == SBK_SUCCESS);

    if (!g_registeredReflection)
    {
        sbk::reflection::registerReflectionTypes();
        g_registeredReflection = true;
    }

    g_system->m_listenerGameObject = g_system->create_database_object<sbk::engine::game_object>().get();
    g_system->m_listenerGameObject->set_object_name("Listener");
    g_system->m_listenerGameObject->set_editor_hidden(true);

    // TODO
    // Add way of turning off profiling
    g_system->m_voiceTracker = std::make_unique<profiling::voice_tracker>();

    g_system->m_studioThreadTimer = g_system->m_threadRuntime->timer_queue()->make_timer(0ms, 20ms, g_system->get_background_thread_executor(),
                                                         [] { g_system->update_async(); });

    return result;
}

auto system::update() -> sbk_result
{
    ZoneScoped;

    if (g_system == nullptr)
    {
        return SBK_ERR_BAKERY_UNINITIALIZED;
    }

    if (g_system->m_voiceTracker)
    {
        g_system->m_voiceTracker->update(g_system.get());
    }

    g_system->m_gameThreadExecuter->loop(g_system->m_gameThreadExecuter->size());

    return SBK_SUCCESS;
}

auto sbk::engine::system::update_async() -> concurrencpp::result<void>
{
    FrameMarkStart(profiling_strings::g_updateName);
    ZoneScoped;

    co_await update_commands();
    co_await update_objects();
    co_await update_profiling();

    FrameMarkEnd(profiling_strings::g_updateName);
    co_return;
}

auto sbk::engine::system::update_commands() -> concurrencpp::result<void>
{
    m_studioThreadExecuter->loop(m_studioThreadExecuter->size());
    co_return;
}

auto sbk::engine::system::update_objects() -> concurrencpp::result<void>
{
    const auto gameObjects = co_await g_system->get_objects_of_type(sbk::engine::game_object::type());

    for (auto& object : gameObjects)
    {
        if (object)
        {
            if (sbk::engine::game_object* const gameObject = object->try_convert_object<sbk::engine::game_object>())
            {
                gameObject->update();
            }
        }
    }
    co_return;
}

auto sbk::engine::system::update_profiling() -> concurrencpp::result<void>
{
    TracyPlotConfig(profiling_strings::g_gameObjectPlotName, tracy::PlotFormatType::Number, true, false, 0);
    TracyPlotConfig(profiling_strings::g_nodeInstancePlotName, tracy::PlotFormatType::Number, true, false, 0);
    TracyPlotConfig(profiling_strings::g_voicePlotName, tracy::PlotFormatType::Number, true, false, 0);

    const std::size_t gameObjectsCount = co_await g_system->get_objects_of_type_size(sbk::engine::game_object::type());
    const std::size_t voiceCount        = co_await g_system->get_objects_of_type_size(sbk::engine::voice::type());
    const std::size_t nodeInstanceCount = co_await g_system->get_objects_of_type_size(sbk::engine::node_instance::type());

    TracyPlot(profiling_strings::g_gameObjectPlotName, (int64_t)nodeInstanceCount);
    TracyPlot(profiling_strings::g_voicePlotName, (int64_t)voiceCount);
    TracyPlot(profiling_strings::g_nodeInstancePlotName, (int64_t)nodeInstanceCount);

    rpmalloc_global_statistics_t stats;
    rpmalloc_global_statistics(&stats);

    TracyPlotConfig(profiling_strings::g_totalMemory, tracy::PlotFormatType::Memory, true, true, 0);
    TracyPlot(profiling_strings::g_totalMemory, (int64_t)stats.mapped_total);

    TracyPlotConfig(profiling_strings::g_currentMemory, tracy::PlotFormatType::Memory, true, true, 0);
    TracyPlot(profiling_strings::g_currentMemory, (int64_t)stats.mapped);
}

auto system::get_current_object_owner() -> sbk::core::object_owner* { return m_project.get(); }

auto sbk::engine::system::post_event(const char* eventName, sbk_id gameObjectID) -> sbk_result
{
    ZoneScoped;
    SC_CHECK(g_system != nullptr, SBK_ERR_BAKERY_UNINITIALIZED);
    SC_CHECK_ARG(eventName);

    std::weak_ptr<sbk::core::database_object> event = g_system->try_find_database_object(sbk::core::database_name(eventName)).get();
    SC_CHECK(!event.expired(), SBK_ERR_BAKERY_OBJECT_NOT_FOUND);

    std::weak_ptr<sbk::core::database_object> gameObject = get_game_object(gameObjectID).get();
    SC_CHECK(!gameObject.expired(), SBK_ERR_BAKERY_OBJECT_NOT_FOUND);

    g_system->get_system_thread_executor()->post([event, gameObject]() 
        {
            if (std::shared_ptr<sbk::engine::game_object> sharedGameObject = std::static_pointer_cast<sbk::engine::game_object>(gameObject.lock()))
            {
                if (std::shared_ptr<sbk::engine::event> sharedEvent = std::static_pointer_cast<sbk::engine::event>(event.lock()))
                {
                    sharedGameObject->post_event(sharedEvent.get(), pass_key<sbk::engine::system>());
                }
            }
        }
    );

    return SBK_SUCCESS;
}

auto sbk::engine::system::post_container(sbk_id containerID, sbk_id gameObjectID) -> sbk_result
{
    ZoneScoped;
    SC_CHECK(g_system != nullptr, SBK_ERR_BAKERY_UNINITIALIZED);
    SC_CHECK_ARG(containerID != 0);

    const std::weak_ptr<sbk::core::database_object> container = g_system->try_find_database_object(containerID).get();
    SC_CHECK(!container.expired(), SBK_ERR_BAKERY_OBJECT_NOT_FOUND);

    const std::weak_ptr<sbk::core::database_object> gameObject = get_game_object(gameObjectID).get();
    SC_CHECK(!gameObject.expired(), SBK_ERR_BAKERY_OBJECT_NOT_FOUND);

    g_system->get_system_thread_executor()->post(
        [container, gameObject]()
        {
            if (const std::shared_ptr<sbk::engine::game_object> sharedGameObject =
                    std::static_pointer_cast<sbk::engine::game_object>(gameObject.lock()))
            {
                if (const std::shared_ptr<sbk::engine::container> sharedContainer =
                        std::static_pointer_cast<sbk::engine::container>(container.lock()))
                {
                    sharedGameObject->play_container(sharedContainer.get(), pass_key<sbk::engine::system>());
                }
            }
        });

    return SBK_SUCCESS;
}

auto sbk::engine::system::stop_all(sbk_id gameObjectID) -> sbk_result
{
    ZoneScoped;
    SC_CHECK(g_system != nullptr, SBK_ERR_BAKERY_UNINITIALIZED);

    const std::weak_ptr<sbk::core::database_object> gameObject = get_game_object(gameObjectID).get();
    SC_CHECK(!gameObject.expired(), SBK_ERR_BAKERY_OBJECT_NOT_FOUND);

    g_system->get_system_thread_executor()->post([gameObject]()
        {
            if (const std::shared_ptr<sbk::engine::game_object> sharedGameObject =
                    std::static_pointer_cast<sbk::engine::game_object>(gameObject.lock()))
            {
                sharedGameObject->stop_all(pass_key<sbk::engine::system>());
            }
        });

    return SBK_SUCCESS;
}

auto sbk::engine::system::get_game_object(sbk_id gameObjectID) -> concurrencpp::result<std::weak_ptr<sbk::core::database_object>>
{
    co_return gameObjectID == 0 ? std::static_pointer_cast<sbk::core::database_object, sbk::engine::game_object>(g_system->m_listenerGameObject) : co_await g_system->try_find_database_object(gameObjectID);
}

auto system::open_project(const std::filesystem::path& projectFile) -> sbk_result
{
    destroy();

    // Create the global logger before initializing Sound Bakery

    const sbk::editor::project_configuration tempProject = sbk::editor::project_configuration(projectFile);

    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    consoleSink->set_level(spdlog::level::info);

    auto now          = spdlog::log_clock::now();
    const time_t tnow = spdlog::log_clock::to_time_t(now);
    const tm now_tm   = spdlog::details::os::localtime(tnow);

    auto dailySink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(
        (tempProject.log_folder() / (std::string(tempProject.project_name()) + ".txt")).string(), now_tm.tm_hour,
        now_tm.tm_min, true, 0, spdlog::file_event_handlers{});
    dailySink->set_level(spdlog::level::trace);

    auto basicFileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
        (tempProject.log_folder() / (std::string(tempProject.project_name()) + ".txt")).string(), true);
    basicFileSink->set_level(spdlog::level::trace);

    const std::shared_ptr<spdlog::logger> logger = std::make_shared<spdlog::logger>(
        std::string("LogSoundBakery"), spdlog::sinks_init_list{consoleSink, dailySink, basicFileSink});
    logger->set_level(spdlog::level::debug);
    logger->set_pattern("[%Y-%m-%d %H:%M:%S %z][Thread %t][%l] %n: %v");

    const std::shared_ptr<spdlog::logger> soundChefLogger = std::make_shared<spdlog::logger>(
        g_soundChefLoggerName, spdlog::sinks_init_list{consoleSink, dailySink, basicFileSink});
    soundChefLogger->set_level(spdlog::level::debug);

    spdlog::set_default_logger(logger);
    spdlog::register_logger(soundChefLogger);

    create();

    const std::string pluginFolder      = tempProject.plugin_folder().string();
    const sbk_system_config systemConfig = sbk_system_config_init(pluginFolder.c_str());

    init(systemConfig);

    g_system->m_project = std::make_unique<sbk::editor::project>();

    if (g_system->m_project->open_project(projectFile).get())
    {
        return SBK_SUCCESS;
    }

    g_system->m_project.reset();

    return SBK_ERR_BAKERY;
}

auto sbk::engine::system::create_project(const std::filesystem::directory_entry& projectDirectory,
                                         const std::string& projectName) -> sbk_result
{
    const sbk::editor::project_configuration projectConfig(projectDirectory, projectName);

    if (open_project(projectConfig.project_file()) == SBK_SUCCESS)
    {
        if (g_system->m_project)
        {
            if (const std::shared_ptr<sbk::engine::bus> masterBus =
                    g_system->m_project->create_database_object<sbk::engine::bus>().get())
            {
                masterBus->set_object_name("Master Bus");
                masterBus->setMasterBus(true);

                g_system->m_project->save_project();

                return SBK_SUCCESS;
            }
        }
    }

    return SBK_ERR_BAKERY;
}

auto sbk::engine::system::load_soundbank(const std::filesystem::path& file, sbk_id& outID) -> sbk_result
{
    SC_CHECK_ARG(std::filesystem::exists(file));
    SC_CHECK(g_system != nullptr, SBK_ERR_BAKERY_UNINITIALIZED);

    sbk::core::serialization::binary_serializer binarySerializer;
    outID = binarySerializer.load_object<sbk::core::serialization::serialized_soundbank>(g_system.get(), file).get();
    return outID != SBK_INVALID_ID ? SBK_SUCCESS : SBK_ERR_BAKERY_SERIALIZATION;
}

auto system::get_project() -> sbk::editor::project*
{
    if (g_system != nullptr)
    {
        return g_system->m_project.get();
    }

    return nullptr;
}

auto system::get_voice_tracker() -> sbk::engine::profiling::voice_tracker*
{
    if (g_system != nullptr)
    {
        return g_system->m_voiceTracker.get();
    }

    return nullptr;
}

auto sbk::engine::system::get_game_thread_executor() const -> std::shared_ptr<concurrencpp::manual_executor>
{
    return m_gameThreadExecuter;
}

auto sbk::engine::system::get_system_thread_executor() const -> std::shared_ptr<concurrencpp::manual_executor>
{
    return m_studioThreadExecuter;
}

auto sbk::engine::system::get_thread_pool_executor() const -> std::shared_ptr<concurrencpp::thread_pool_executor>
{
    return m_threadRuntime ? m_threadRuntime->thread_pool_executor() : std::shared_ptr<concurrencpp::thread_pool_executor>{};
}

auto sbk::engine::system::get_background_thread_executor() const -> std::shared_ptr<concurrencpp::thread_pool_executor>
{
    return m_threadRuntime ? m_threadRuntime->background_executor() : std::shared_ptr<concurrencpp::thread_pool_executor>{};
}

auto sbk::engine::system::get_database_executor() const -> std::shared_ptr<concurrencpp::worker_thread_executor>
{
    return m_workerThread;
}

auto sbk::engine::system::get_listener_game_object() const -> sbk::engine::game_object*
{
    return m_listenerGameObject.get(); 
}

auto sbk::engine::system::get_master_bus() const -> sbk::engine::bus* { return m_masterBus.get(); }

auto sbk::engine::system::set_master_bus(const std::shared_ptr<sbk::engine::bus>& masterBus) -> void
{
    BOOST_ASSERT(!m_masterBus);
    m_masterBus = masterBus;
}
