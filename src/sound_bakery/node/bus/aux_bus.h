#pragma once

#include "bus.h"

namespace sbk::engine
{
    class SB_CLASS AuxBus : public Bus
    {
        REGISTER_REFLECTION(AuxBus, Bus)
    };
}  // namespace sbk::engine