#include "system.h"

#include "sound_bakery/core/object/object_global.h"
#include "sound_bakery/editor/project/project.h"
#include "sound_bakery/factory.h"
#include "sound_bakery/gameobject/gameobject.h"
#include "sound_bakery/node/bus/bus.h"
#include "sound_bakery/profiling/voice_tracker.h"
#include "sound_bakery/reflection/reflection.h"
#include "spdlog/sinks/daily_file_sink.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

using namespace SB::Engine;

System::~System() = default;

namespace
{
    static System* s_system = nullptr;

    static const std::string s_soundChefLoggerName("LogSoundChef");
}  // namespace

static void miniaudioLogCallback(void* pUserData, ma_uint32 level, const char* pMessage)
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

System::System() : m_listenerGameObject(nullptr)
{
    assert(s_system == nullptr);
    s_system = this;

    sc_system* chefSystem = nullptr;

    sc_result createResult = sc_system_create(&chefSystem);
    assert(createResult == MA_SUCCESS);

    if (createResult == MA_SUCCESS)
    {
        m_chefSystem.reset(chefSystem);
    }

    sc_result initLogResult = sc_system_log_init(chefSystem, miniaudioLogCallback);
    assert(initLogResult == MA_SUCCESS);
}

System* System::get() { return s_system; }

sc_system* System::getChef()
{
    System* system = get();
    return system ? system->m_chefSystem.get() : nullptr;
}

System* System::create()
{
    if (s_system == nullptr)
    {
        s_system = new System();

        if (s_system)
        {
            s_system->m_objectTracker = std::make_unique<SB::Core::ObjectTracker>();
            s_system->m_database      = std::make_unique<SB::Core::Database>();

            s_system->mainThreadExecuter = s_system->concurrenRuntime.make_manual_executor();
        }
    }

    return s_system;
}

void System::destroy()
{
    if (s_system != nullptr)
    {
        spdlog::debug("Closing Sound Bakery");

        // Close threads
        s_system->getBackgroundExecuter()->shutdown();
        s_system->mainThreadExecuter->shutdown();

        s_system->m_listenerGameObject->stopAll();
        s_system->m_listenerGameObject.reset();

        s_system->m_database->clear();

        SB::Reflection::unregisterReflectionTypes();

        s_system->m_chefSystem.reset();

        spdlog::shutdown();

        delete s_system;
        s_system = nullptr;
    }
}

SB_RESULT System::init()
{
    if (s_system == nullptr)
    {
        return MA_DEVICE_NOT_STARTED;
    }

    sc_result result = sc_system_init(s_system->m_chefSystem.get());
    assert(result == MA_SUCCESS);

    SB::Reflection::registerReflectionTypes();

    // TODO
    // Add way of turning off profiling
    s_system->m_voiceTracker = std::make_unique<Profiling::VoiceTracker>();

    return result;
}

SB_RESULT System::update()
{
    if (s_system == nullptr)
    {
        return MA_DEVICE_NOT_STARTED;
    }

    if (s_system->m_voiceTracker)
    {
        s_system->m_voiceTracker->update(s_system);
    }

    s_system->m_listenerGameObject->update();

    s_system->mainThreadExecuter->loop(32);

    return MA_SUCCESS;
}

SB_RESULT System::openProject(const std::filesystem::path& projectFile)
{
    destroy();

    // Create the global logger before initializing Sound Bakery

    const SB::Editor::ProjectConfiguration tempProject = SB::Editor::ProjectConfiguration(projectFile);

    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    consoleSink->set_level(spdlog::level::info);

    auto now    = spdlog::log_clock::now();
    time_t tnow = spdlog::log_clock::to_time_t(now);
    tm now_tm   = spdlog::details::os::localtime(tnow);

    auto dailySink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(
        (tempProject.logFolder() / (tempProject.m_projectName + ".txt")).string(), now_tm.tm_hour, now_tm.tm_min, true,
        0, spdlog::file_event_handlers{});
    dailySink->set_level(spdlog::level::trace);

    auto basicFileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
        (tempProject.logFolder() / (tempProject.m_projectName + ".txt")).string(), true);
    basicFileSink->set_level(spdlog::level::trace);

    std::shared_ptr<spdlog::logger> logger = std::make_shared<spdlog::logger>(
        std::string("LogSoundBakery"), spdlog::sinks_init_list{consoleSink, dailySink, basicFileSink});
    logger->set_level(spdlog::level::debug);
    logger->set_pattern("[%Y-%m-%d %H:%M:%S %z][Thread %t][%l] %n: %v");

    std::shared_ptr<spdlog::logger> soundChefLogger = std::make_shared<spdlog::logger>(
        s_soundChefLoggerName, spdlog::sinks_init_list{consoleSink, dailySink, basicFileSink});
    soundChefLogger->set_level(spdlog::level::debug);

    spdlog::set_default_logger(logger);
    spdlog::register_logger(soundChefLogger);

    create();
    init();

    s_system->m_project = std::make_unique<SB::Editor::Project>();

    if (s_system->m_project->openProject(projectFile))
    {
        return MA_SUCCESS;
    }

    s_system->m_project.reset();

    return MA_ERROR;
}

SB::Core::ObjectTracker* System::getObjectTracker()
{
    if (s_system)
    {
        return s_system->m_objectTracker.get();
    }

    return nullptr;
}

SB::Core::Database* System::getDatabase()
{
    if (s_system)
    {
        return s_system->m_database.get();
    }

    return nullptr;
}

SB::Editor::Project* System::getProject()
{
    if (s_system)
    {
        return s_system->m_project.get();
    }

    return nullptr;
}

void SB::Engine::System::onLoaded()
{
    m_listenerGameObject = std::unique_ptr<GameObject>(newObject<GameObject>());

    if (!m_masterBus.lookup())
    {
        createMasterBus();
    }

    // for (auto& object : m_objects)
    //{
    //     //object.second->onProjectLoaded();
    // }
}

void SB::Engine::System::createMasterBus()
{
    assert(m_masterBus.null() && "Shouldn't create a master bus when one exists");

    m_masterBus = newDatabaseObject<Bus>();
    m_masterBus->setDatabaseName("Master Bus");
    m_masterBus->setMasterBus(true);
}

void System::setMasterBus(const SB::Core::DatabasePtr<Bus>& bus)
{
    if (m_masterBus.lookup())
    {
        m_masterBus->setMasterBus(false);
    }

    m_masterBus = bus;

    if (m_masterBus.lookup())
    {
        m_masterBus->setMasterBus(true);
    }
}

GameObject* System::getListenerGameObject() const { return m_listenerGameObject.get(); }
