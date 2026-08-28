#include "bus.h"

#include "sound_bakery/system.h"
#include "sound_bakery/runtime/runtime.h"

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

auto sbk::engine::bus::lock_or_copy_node_group() -> sbk::result<std::shared_ptr<sc_node_group>>
{
    if (!m_nodeGroup)
    {
        sc_node_group* nodeGroup{};
        SBK_TRY_C(sc_system_create_node_group(get_runtime(), &nodeGroup));
        m_nodeGroup = std::shared_ptr<sc_node_group>(nodeGroup, SC_NODE_GROUP_DELETER{});
    }

    return m_nodeGroup;
}
