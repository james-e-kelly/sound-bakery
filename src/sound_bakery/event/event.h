#pragma once

#include "sound_bakery/core/core_include.h"

namespace sbk::engine
{
    enum class action_type
    {
        invalid,
        play,
        stop,
        num
    };

    struct SB_CLASS action
    {
        action_type m_type = action_type::play;
        sbk::core::database_ptr<sbk::core::database_object> m_destination;
    };

    class SB_CLASS event : public sbk::core::database_object
    {
        REGISTER_REFLECTION(event, sbk::core::database_object)

    public:
        eastl::vector<action> m_actions;
    };
}  // namespace sbk::engine