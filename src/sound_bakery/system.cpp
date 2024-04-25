#include "system.h"

#include "sound_bakery/core/object/object_global.h"
#include "sound_bakery/editor/project/project.h"
#include "sound_bakery/factory.h"
#include "sound_bakery/gameobject/gameobject.h"
#include "sound_bakery/node/bus/bus.h"
#include "sound_bakery/profiling/voice_tracker.h"
#include "sound_bakery/reflection/reflection.h"

using namespace SB::Engine;

System::~System() = default;

namespace
{
    static System* s_system = nullptr;
}

void* operator new[](size_t size, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
{
    (void)pName;
    (void)flags;
    (void)debugFlags;
    (void)file;
    (void)line;

    return std::malloc(size);
}

void* operator new[](size_t size,
                     size_t alignment,
                     size_t alignmentOffset,
                     const char* pName,
                     int flags,
                     unsigned debugFlags,
                     const char* file,
                     int line)
{
    (void)alignment;
    (void)alignmentOffset;
    (void)pName;
    (void)flags;
    (void)debugFlags;
    (void)file;
    (void)line;

    return std::malloc(size);
}

System::System() : m_listenerGameObject(nullptr)
{
    assert(s_system == nullptr);
    s_system = this;

    SC_SYSTEM* chefSystem = nullptr;

    SC_RESULT createResult = SC_System_Create(&chefSystem);
    assert(createResult == MA_SUCCESS);

    if (createResult == MA_SUCCESS)
    {
        m_chefSystem.reset(chefSystem);
    }
}

System* System::get() { return s_system; }

SC_SYSTEM* System::getChef()
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
            s_system->m_project       = std::make_unique<SB::Editor::Project>();
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

    SC_RESULT result = SC_System_Init(s_system->m_chefSystem.get());
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

    return s_system->m_project->openProject(projectFile) ? MA_SUCCESS : MA_INVALID_FILE;
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
