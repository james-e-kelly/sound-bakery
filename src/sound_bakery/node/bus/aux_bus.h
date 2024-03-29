#pragma once

#include "bus.h"

namespace SB::Engine
{
    class SB_CLASS AuxBus : public Bus
    {
        REGISTER_REFLECTION(AuxBus, Bus)
    };
}  // namespace SB::Engine