#include "container.h"

#include "sound_bakery/node/bus/bus.h"
#include "sound_bakery/system.h"

using namespace sbk::engine;

DEFINE_REFLECTION(sbk::engine::container)

auto sbk::engine::container::can_add_child_type(const rttr::type& childType) const -> bool
{
    return sbk::engine::node_base::can_add_child_type(childType) && childType.is_derived_from<sbk::engine::container>();
}