#pragma once

#include "sound_bakery/node/container/container.h"

namespace SB::Engine
{
    class SequenceContainer : public Container
    {
    public:
        virtual void gatherSounds(GatherSoundsContext& context) override
        {
        }

        RTTR_ENABLE(Container)
        RTTR_REGISTRATION_FRIEND
    };
}  // namespace SB::Engine