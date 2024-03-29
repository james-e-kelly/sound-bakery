#pragma once

#include "sound_bakery/node/container/container.h"

namespace SB::Engine
{
    class SB_CLASS BlendContainer : public Container
    {
    public:
        virtual void gatherChildrenForPlay(GatherChildrenContext& context) const override;

        REGISTER_REFLECTION(BlendContainer, Container)
        RTTR_REGISTRATION_FRIEND
    };
}  // namespace SB::Engine