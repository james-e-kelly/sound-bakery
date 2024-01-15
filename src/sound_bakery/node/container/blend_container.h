#pragma once

#include "sound_bakery/node/container/container.h"

namespace SB::Engine
{
    class BlendContainer : public Container
    {
    public:
        virtual void gatherSounds(
            std::vector<Container*>& soundContainers,
            const RuntimeFloatParameterMap& runtimeFloatParameters,
            const RuntimeIntParameterMap& runtimeIntParameters) override;

        RTTR_ENABLE(Container)
        RTTR_REGISTRATION_FRIEND
    };
}  // namespace SB::Engine