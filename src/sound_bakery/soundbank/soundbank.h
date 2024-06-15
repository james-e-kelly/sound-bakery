#pragma once

#include "sound_bakery/core/core_include.h"

namespace sbk::engine
{
    class Event;

    class SB_CLASS Soundbank : public sbk::core::database_object
    {
        REGISTER_REFLECTION(Soundbank, sbk::core::database_object)

    public:
        std::vector<sbk::core::DatabasePtr<Event>> GetEvents() const { return m_events; }

    private:
        std::vector<sbk::core::DatabasePtr<Event>> m_events;
    };
}  // namespace SB::Engine