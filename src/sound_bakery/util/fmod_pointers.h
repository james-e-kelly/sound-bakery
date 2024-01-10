#pragma once

#include "sound_chef/sound_chef.h"

namespace SB
{
    struct SC_SYSTEM_DELETER
    {
        void operator()(SC_SYSTEM* system)
        {
            SC_System_Close(system);
            SC_System_Release(system);
        }
    };

    using SystemPtr = std::unique_ptr<SC_SYSTEM, SC_SYSTEM_DELETER>;
}