#include "bus.h"

#include "sound_bakery/system.h"
#include "sound_bakery/voice/node_instance.h"

using namespace SB::Engine;

DEFINE_REFLECTION(SB::Engine::Bus)

void Bus::onLoaded()
{
    Node::onLoaded();

    if (SB::Engine::System* system = SB::Engine::System::get())
    {
        if (system->getMasterBus().id() == getDatabaseID())
        {
            setMasterBus(true);
        }
    }
}

void Bus::lock()
{
    if (!m_busInstance)
    {
        m_busInstance = std::make_shared<NodeInstance>();

        InitNodeInstance initData;
        initData.refNode = tryConvertObject<NodeBase>();
        initData.type    = NodeInstanceType::BUS;

        m_busInstance->init(initData);
    }
}

void SB::Engine::Bus::unlock() { m_busInstance.reset(); }

std::shared_ptr<NodeInstance> Bus::lockAndCopy()
{
    lock();
    return m_busInstance;
}
