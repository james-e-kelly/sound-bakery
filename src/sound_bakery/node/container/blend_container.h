#pragma once

#include "sound_bakery/node/container/container.h"

namespace sbk::engine
{
    class SB_CLASS BlendContainer : public container
    {
    public:
        virtual void gather_children_for_play(gather_children_context& context) const override;

        REGISTER_REFLECTION(BlendContainer, container)
        RTTR_REGISTRATION_FRIEND
    };
}  // namespace sbk::engine