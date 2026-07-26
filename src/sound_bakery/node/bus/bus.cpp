#include "bus.h"

#include "sound_bakery/system.h"
#include "sound_bakery/voice/node_instance.h"

using namespace sbk::engine;

DEFINE_REFLECTION(sbk::engine::bus)

auto sbk::engine::bus::can_add_child_type(const rttr::type& childType) const -> bool
{
    return sbk::engine::node_base::can_add_child_type(childType) && childType.is_derived_from<sbk::engine::bus>();
}

auto sbk::engine::bus::can_add_parent() const -> bool { return sbk::engine::node_base::can_add_parent() && !m_masterBus; }

auto sbk::engine::bus::can_add_parent_type(const rttr::type& parentType) const -> bool
{
    // Busses can only have bus parents
    return sbk::engine::node_base::can_add_parent_type(parentType) && parentType == sbk::engine::bus::type();
}

auto sbk::engine::bus::set_master_bus(bool isMaster) -> void
{
    if (get_object_type() == rttr::type::get<bus>())
    {
        m_masterBus = isMaster;
    }
}

auto bus::lock_and_copy() -> std::shared_ptr<node_instance>
{
    if (m_busInstance.expired())
    {
        std::shared_ptr<node_instance> sharedBus = std::make_shared<node_instance>();

        event_init initData;
        initData.refNode = try_convert_object<node_base>();
        initData.type    = node_instance_type::bus;

        if (sharedBus->init(initData).has_value())
        {
            m_busInstance = sharedBus;
        }
    }

    return m_busInstance.lock();
}
