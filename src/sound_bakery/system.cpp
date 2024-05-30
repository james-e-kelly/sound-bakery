#include "system.h"

#include "sound_bakery/core/object/object_global.h"
#include "sound_bakery/editor/project/project.h"
#include "sound_bakery/factory.h"
#include "sound_bakery/gameobject/gameobject.h"
#include "sound_bakery/node/bus/bus.h"
#include "sound_bakery/profiling/voice_tracker.h"
#include "sound_bakery/reflection/reflection.h"

#include "spdlog/sinks/stdout_color_sinks.h"

using namespace SB::Engine;

System::~System() = default;

namespace
{
    static System* s_system = nullptr;
}

void miniaudioLogCallback(void* pUserData, ma_uint32 level, const char* pMessage)
{
    (void)pUserData;

    switch (level)
    {
        case MA_LOG_LEVEL_DEBUG:
            spdlog::debug("[BAKERY]: {}", pMessage);
            break;
        case MA_LOG_LEVEL_INFO:
            spdlog::info("[BAKERY]: {}", pMessage);
            break;
        case MA_LOG_LEVEL_WARNING:
            spdlog::warn("[BAKERY]: {}", pMessage);
            break;
        case MA_LOG_LEVEL_ERROR:
            spdlog::error("[BAKERY]: {}", pMessage);
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
        // Store messages and later dump them when the project is loaded
        spdlog::enable_backtrace(128);

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
        s_system->m_listenerGameObject->stopAll();
        s_system->m_listenerGameObject.reset();

        s_system->m_database->clear();

        SB::Reflection::unregisterReflectionTypes();

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
    if (s_system == nullptr)
    {
        return MA_DEVICE_NOT_STARTED;
    }

    s_system->m_project.reset();

    s_system->m_project = std::make_unique<SB::Editor::Project>();

    if (s_system->m_project->openProject(projectFile))
    {
        auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        consoleSink->set_level(spdlog::level::info);
        consoleSink->set_pattern("[multi_sink_example] [%^%l%$] %v");

        auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>((s_system->m_project->getConfig().logFolder() / "log.txt").string(), true);
        fileSink->set_level(spdlog::level::trace);

        spdlog::logger logger("multi_sink", {consoleSink, fileSink});
        logger.set_level(spdlog::level::debug);

        s_system->logger = std::make_shared<spdlog::logger>(std::string("multi_sink"), spdlog::sinks_init_list{consoleSink, fileSink});
        
        spdlog::set_default_logger(s_system->logger);

        spdlog::dump_backtrace();

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
