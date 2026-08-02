#pragma once

#include "sound_bakery/node/container/container.h"

namespace sbk::engine
{
    class SB_CLASS sequence_container : public container
    {
    public:
        virtual auto gather_children_for_play(gather_children_context& context) const -> void override;

    private:
        eastl::vector<sbk::core::child_ptr<container>> m_sequence;

        REGISTER_REFLECTION(sequence_container, container)
        RTTR_REGISTRATION_FRIEND
    };
}  // namespace sbk::engine