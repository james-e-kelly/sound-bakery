#pragma once

#include "bus.h"

namespace sbk::engine
{
    class SB_CLASS aux_bus : public bus
    {
        REGISTER_REFLECTION(aux_bus, bus)
    };
}  // namespace sbk::engine