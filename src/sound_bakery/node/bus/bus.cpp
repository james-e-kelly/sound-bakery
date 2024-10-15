#include "bus.h"

#include "sound_bakery/system.h"
#include "sound_bakery/voice/node_instance.h"

using namespace sbk::engine;

DEFINE_REFLECTION(sbk::engine::bus)

bool sbk::engine::bus::can_add_child_type(const rttr::type& childType) const
{
    return sbk::engine::node_base::can_add_child_type(childType) && childType.is_derived_from<sbk::engine::bus>();
}

bool sbk::engine::bus::can_add_parent() const { return sbk::engine::node_base::can_add_parent() && !m_masterBus; }

bool sbk::engine::bus::can_add_parent_type(const rttr::type& parentType) const
{
    // Busses can only have bus parents
    return sbk::engine::node_base::can_add_parent_type(parentType) && parentType == sbk::engine::bus::type();
}

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
        m_busInstance = std::make_shared<node_instance>();

        InitNodeInstance initData;
        initData.refNode = try_convert_object<node_base>();
        initData.type    = NodeInstanceType::BUS;

        m_busInstance->init(initData);
    }
}

void sbk::engine::bus::unlock() { m_busInstance.reset(); }

std::shared_ptr<node_instance> bus::lockAndCopy()
{
    lock();
    return m_busInstance;
}
