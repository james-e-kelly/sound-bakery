#include "system.h"

#include "sound_bakery/editor/project/project.h"
#include "sound_bakery/gameobject/gameobject.h"
#include "sound_bakery/node/bus/bus.h"
#include "sound_bakery/profiling/voice_tracker.h"
#include "sound_bakery/reflection/reflection.h"
#include "sound_bakery/serialization/serializer.h"
#include "spdlog/sinks/daily_file_sink.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

using namespace sbk::engine;

namespace
{
    sbk::engine::system* s_system = nullptr;
    bool s_registeredReflection   = false;

    const std::string s_soundChefLoggerName("LogSoundChef");

    void miniaudio_log_callback(void* pUserData, ma_uint32 level, const char* pMessage)
    {
        (void)pUserData;

        auto soundChefLogger = spdlog::get(s_soundChefLoggerName);

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

system::system() : sc_system(), m_gameThreadExecuter(make_manual_executor())
{
    BOOST_ASSERT(s_system == nullptr);
    s_system = this;

    const sc_result initLogResult = sc_system_log_init(this, miniaudio_log_callback);
    BOOST_ASSERT(initLogResult == MA_SUCCESS);
}

system::~system()
{
    SPDLOG_DEBUG("Closing Sound Bakery");

    // Close threads
    background_executor()->shutdown();
    get_game_thread_executer()->shutdown();

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

    destroy_all();
    BOOST_ASSERT(get_objects_count() == 0);

    sc_system_close(this);

    spdlog::shutdown();
}

sbk::engine::system* system::get() { return s_system; }

auto sbk::engine::system::get_operating_mode() -> operating_mode
{
    if (s_system)
    {
        if (s_system->m_project)
        {
            return operating_mode::editor;
        }

        if (s_system->get_objects_count())
        {
            return operating_mode::runtime;
        }
    }

    return operating_mode::unkown;
}

sbk::engine::system* system::create()
{
    if (s_system == nullptr)
    {
        s_system = new system();
    }

    return s_system;
}

void system::destroy()
{
    if (s_system != nullptr)
    {
        delete s_system;
        s_system = nullptr;
    }
}

sc_result system::init(const sb_system_config& config)
{
    if (s_system == nullptr)
    {
        return MA_DEVICE_NOT_STARTED;
    }

    const sc_result result = sc_system_init(s_system, &config.soundChefConfig);
    BOOST_ASSERT(result == MA_SUCCESS);

    if (!s_registeredReflection)
    {
        sbk::reflection::registerReflectionTypes();
        s_registeredReflection = true;
    }

    s_system->m_listenerGameObject = s_system->create_runtime_object<sbk::engine::game_object>();

    // TODO
    // Add way of turning off profiling
    s_system->m_voiceTracker = std::make_unique<profiling::voice_tracker>();

    return result;
}

sc_result system::update()
{
    if (s_system == nullptr)
    {
        return MA_DEVICE_NOT_STARTED;
    }

    if (s_system->m_voiceTracker)
    {
        s_system->m_voiceTracker->update(s_system);
    }

    std::unordered_set<sbk::core::object*> objects = s_system->get_objects_of_type(sbk::engine::game_object::type());

    std::for_each(
        objects.begin(), objects.end(),
        [](sbk::core::object* const object)
        {
            if (object != nullptr)
            {
                if (sbk::engine::game_object* const gameObject = object->try_convert_object<sbk::engine::game_object>())
                {
                    gameObject->update();
                }
            }
        });

    s_system->m_gameThreadExecuter->loop(32);

    return MA_SUCCESS;
}

sbk::core::object_owner* system::get_current_object_owner() { return m_project.get(); }

sc_result system::open_project(const std::filesystem::path& project_file)
{
    destroy();

    // Create the global logger before initializing Sound Bakery

    const sbk::editor::project_configuration tempProject = sbk::editor::project_configuration(project_file);

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
        s_soundChefLoggerName, spdlog::sinks_init_list{consoleSink, dailySink, basicFileSink});
    soundChefLogger->set_level(spdlog::level::debug);

    spdlog::set_default_logger(logger);
    spdlog::register_logger(soundChefLogger);

    create();

    const std::string pluginFolder      = tempProject.plugin_folder().string();
    const sb_system_config systemConfig = sb_system_config_init(pluginFolder.c_str());

    init(systemConfig);

    s_system->m_project = std::make_unique<sbk::editor::project>();

    if (s_system->m_project->open_project(project_file))
    {
        return MA_SUCCESS;
    }

    s_system->m_project.reset();

    return MA_ERROR;
}

sc_result sbk::engine::system::create_project(const std::filesystem::directory_entry& projectDirectory,
                                              const std::string& projectName)
{
    const sbk::editor::project_configuration projectConfig(projectDirectory, projectName);

    if (open_project(projectConfig.project_file()) == MA_SUCCESS)
    {
        if (s_system->m_project)
        {
            if (std::shared_ptr<sbk::engine::bus> masterBus =
                    s_system->m_project->create_database_object<sbk::engine::bus>())
            {
                masterBus->set_database_name("Master Bus");
                masterBus->setMasterBus(true);

                s_system->m_project->save_project();

                return MA_SUCCESS;
            }
        }
    }

    return MA_ERROR;
}

auto sbk::engine::system::load_soundbank(const std::filesystem::path& file) -> sb_result
{
    SC_CHECK_ARG(std::filesystem::exists(file));
    SC_CHECK(s_system != nullptr, MA_DEVICE_NOT_STARTED);

    sbk::core::serialization::binary_serializer binarySerializer;
    return binarySerializer.load_object<sbk::core::serialization::serialized_soundbank>(s_system, file);
}

sbk::editor::project* system::get_project()
{
    if (s_system != nullptr)
    {
        return s_system->m_project.get();
    }

    return nullptr;
}

sbk::engine::profiling::voice_tracker* system::get_voice_tracker()
{
    if (s_system != nullptr)
    {
        return s_system->m_voiceTracker.get();
    }

    return nullptr;
}

auto sbk::engine::system::get_game_thread_executer() const -> std::shared_ptr<concurrencpp::manual_executor>
{
    return m_gameThreadExecuter;
}

sbk::engine::game_object* sbk::engine::system::get_listener_game_object() const
{
    return m_listenerGameObject.get(); }

auto sbk::engine::system::get_master_bus() const -> sbk::engine::bus* { return m_masterBus.get(); }

void sbk::engine::system::set_master_bus(const std::shared_ptr<sbk::engine::bus>& masterBus)
{
    BOOST_ASSERT(!m_masterBus);
    m_masterBus = masterBus;
}
