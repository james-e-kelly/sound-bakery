#pragma once

#include "sound_bakery/node/container/container.h"

namespace SB::Engine
{
    class BlendContainer : public Container
    {
    public:
        virtual void gatherSounds(std::vector<Container*>& soundContainers) override;

        RTTR_ENABLE(Container)
        RTTR_REGISTRATION_FRIEND
    };
}