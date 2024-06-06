#pragma once

#include "sound_bakery/core/core_include.h"

namespace SB::Engine
{
    class Event;

    class SB_CLASS Soundbank : public SB::Core::DatabaseObject
    {
        REGISTER_REFLECTION(Soundbank, SB::Core::DatabaseObject)

    public:
        std::vector<SB::Core::DatabasePtr<Event>> GetEvents() const { return m_events; }

    private:
        std::vector<SB::Core::DatabasePtr<Event>> m_events;
    };
}  // namespace SB::Engine