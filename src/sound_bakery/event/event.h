#pragma once

#include "sound_bakery/core/core_include.h"

namespace sbk::engine
{
    enum SB_ACTION_TYPE
    {
        SB_ACTION_INVALID,
        SB_ACTION_PLAY,
        SB_ACTION_STOP,
        SB_ACTION_NUM
    };

    struct SB_CLASS action
    {
        SB_ACTION_TYPE m_type = SB_ACTION_PLAY;
        sbk::core::database_ptr<sbk::core::database_object> m_destination;
    };

    class SB_CLASS event : public sbk::core::database_object
    {
        REGISTER_REFLECTION(event, sbk::core::database_object)

    public:
        std::vector<action> m_actions;
    };
}  // namespace sbk::engine