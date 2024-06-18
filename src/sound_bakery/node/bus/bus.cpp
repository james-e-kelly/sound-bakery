#include "bus.h"

#include "sound_bakery/system.h"
#include "sound_bakery/voice/node_instance.h"

using namespace sbk::engine;

DEFINE_REFLECTION(sbk::engine::Bus)

void Bus::onLoaded()
{
    Node::onLoaded();

    // if (sbk::engine::system* system = sbk::engine::system::get())
    //{
    //     if (system->getMasterBus().id() == get_database_id())
    //     {
    //         setMasterBus(true);
    //     }
    // }
}

void Bus::lock()
{
    if (!m_busInstance)
    {
        m_busInstance = std::make_shared<NodeInstance>();

        InitNodeInstance initData;
        initData.refNode = try_convert_object<NodeBase>();
        initData.type    = NodeInstanceType::BUS;

        m_busInstance->init(initData);
    }
}

void sbk::engine::Bus::unlock() { m_busInstance.reset(); }

std::shared_ptr<NodeInstance> Bus::lockAndCopy()
{
    lock();
    return m_busInstance;
}
