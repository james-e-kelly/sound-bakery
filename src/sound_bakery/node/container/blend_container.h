#pragma once

#include "sound_bakery/node/container/container.h"

namespace sbk::engine
{
    class SB_CLASS blend_container : public container
    {
    public:
        virtual auto gather_children_for_play(gather_children_context& context) const -> void override;

        REGISTER_REFLECTION(blend_container, container)
        RTTR_REGISTRATION_FRIEND
    };
}  // namespace sbk::engine