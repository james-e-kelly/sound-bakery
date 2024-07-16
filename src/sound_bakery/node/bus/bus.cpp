#include "bus.h"

#include "sound_bakery/system.h"
#include "sound_bakery/voice/node_instance.h"

using namespace sbk::engine;

DEFINE_REFLECTION(sbk::engine::bus)

void sbk::engine::bus::setMasterBus(bool isMaster)
{
    if (getType() == rttr::type::get<bus>())
    {
        m_masterBus = isMaster;

        if (isMaster)
        {
            sbk::engine::system::get()->set_master_bus(std::static_pointer_cast<sbk::engine::bus>(shared_from_this()));
        }
    }
}

void bus::lock()
{
    if (!m_busInstance)
    {
        m_busInstance = std::make_shared<NodeInstance>();

        InitNodeInstance initData;
        initData.refNode = try_convert_object<node_base>();
        initData.type    = NodeInstanceType::BUS;

        m_busInstance->init(initData);
    }
}

void sbk::engine::bus::unlock() { m_busInstance.reset(); }

std::shared_ptr<NodeInstance> bus::lockAndCopy()
{
    lock();
    return m_busInstance;
}
