#pragma once

#include "sound_chef/sound_chef.h"

namespace SB
{
    struct SC_SYSTEM_DELETER
    {
        void operator()(sc_system* system)
        {
            sc_system_close(system);
            sc_system_release(system);
        }
    };

    using SystemPtr = std::unique_ptr<sc_system, SC_SYSTEM_DELETER>;
}  // namespace SB