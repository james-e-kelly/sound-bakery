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

    struct Action
    {
        SB_ACTION_TYPE m_type = SB_ACTION_PLAY;
        SB::Core::DatabasePtr<SB::Core::DatabaseObject> m_destination;
    };

    class Event : public SB::Core::DatabaseObject
    {
    public:
        std::vector<Action> m_actions;

        RTTR_ENABLE(SB::Core::DatabaseObject)
        RTTR_REGISTRATION_FRIEND
    };
}  // namespace SB::Engine