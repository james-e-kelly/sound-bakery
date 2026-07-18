#include "aux_bus.h"

DEFINE_REFLECTION(sbk::engine::aux_bus)

auto sbk::engine::aux_bus::can_add_parent_type(const rttr::type& parentType) const -> bool
{
    // Aux busses can have bus parents or aux bus parents
    return sbk::engine::node_base::can_add_parent_type(parentType) && parentType.is_derived_from<sbk::engine::bus>();
}

auto sbk::engine::aux_bus::can_add_child_type(const rttr::type& childType) const -> bool
{
    // Aux busses can only have aux bus children
    return sbk::engine::node_base::can_add_child_type(childType) && childType == sbk::engine::aux_bus::type();
}