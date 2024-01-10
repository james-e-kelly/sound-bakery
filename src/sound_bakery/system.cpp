#include "system.h"

#include "sound_bakery/factory.h"
#include "sound_bakery/core/object/object_global.h"
#include "sound_bakery/gameobject/gameobject.h"
#include "sound_bakery/node/bus/bus.h"
#include "sound_bakery/profiling/voice_tracker.h"

using namespace SB::Engine;

static System* s_system = nullptr;

void* operator new[](size_t size, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
{
    return std::malloc(size);
}

void* operator new[](size_t size, size_t alignment, size_t alignmentOffset, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
{
    return std::malloc(size);
}

System::System()
    : m_listenerGameObject(nullptr)
{   
    assert(s_system == nullptr);
    s_system = this;

    SC_RESULT createResult = SC_System_Create(std::out_ptr(m_chefSystem));
    assert(createResult == MA_SUCCESS);   
}

System* System::get()
{
    return s_system;
}

System* System::create()
{
    if (!s_system)
    {
        s_system = new System();
    }

    return s_system;
}

void System::destroy()
{
    if (s_system)
    {
        s_system->m_listenerGameObject->stopAll();
        SB::Core::Database::get()->clear();
        delete s_system;
    }
}

SB_RESULT System::init()
{
    SC_RESULT result = SC_System_Init(m_chefSystem.get());
    assert(result == MA_SUCCESS);

    // TODO
    // Add way of turning off profiling
    m_voiceTracker = std::make_unique<Profiling::VoiceTracker>();

    return result == MA_SUCCESS ? SB_SUCCESS : SB_ERROR;
}

SB_RESULT System::update()
{
    if (m_voiceTracker)
    {
        m_voiceTracker->update(this);
    }

    m_listenerGameObject->update();

    return SB_SUCCESS;
}

void SB::Engine::System::onLoaded()
{
    m_listenerGameObject = std::unique_ptr<GameObject>(newObject<GameObject>());

    if (!m_masterBus.lookup())
    {
        createMasterBus();
    }

    //for (auto& object : m_objects)
    //{
    //    //object.second->onProjectLoaded();
    //}
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

GameObject* System::getListenerGameObject() const
{
    return m_listenerGameObject.get();
}
