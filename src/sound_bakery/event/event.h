#pragma once

#include "sound_bakery/core/core_include.h"

namespace SB::Engine
{
    enum SB_ACTION_TYPE
    {
        SB_ACTION_INVALID,
        SB_ACTION_PLAY,
        SB_ACTION_STOP,
        SB_ACTION_NUM
    };

    struct SB_CLASS Action
    {
        SB_ACTION_TYPE m_type = SB_ACTION_PLAY;
        SB::Core::DatabasePtr<SB::Core::database_object> m_destination;
    };

    class SB_CLASS Event : public SB::Core::database_object
    {
        REGISTER_REFLECTION(Event, SB::Core::database_object)

    public:
        std::vector<Action> m_actions;
    };
}  // namespace SB::Engine