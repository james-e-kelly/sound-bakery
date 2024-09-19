#pragma once

#include "bus.h"

namespace sbk::engine
{
    class SB_CLASS aux_bus : public bus
    {
        REGISTER_REFLECTION(aux_bus, bus)

        bool can_add_parent_type(const rttr::type& parentType) const override;

        bool can_add_child_type(const rttr::type& childType) const override;
    };
}  // namespace sbk::engine