#include "sound_bakery/profiling/property_broadcaster.h"

#include "sound_bakery/core/object/object_owner.h"
#include "sound_bakery/profiling/remote_session.h"

#include <algorithm>

namespace sbk::engine::profiling
{
    auto property_broadcaster::update(remote_session& session, sbk::core::object_owner& objectOwner) -> void
    {
        if (!session.is_connected())
        {
            if (m_wasConnected)
            {
                m_lastSentValues.clear();  //< Shadow state rebuilds on reconnect, re-syncing everything.
                m_sweepCursor = 0;
            }

            m_wasConnected = false;
            return;
        }

        m_wasConnected = true;

        const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

        if (now - m_lastPoll < pollInterval)
        {
            return;
        }

        m_lastPoll = now;

        const std::vector<std::shared_ptr<sbk::core::object>>& objects = objectOwner.get_objects();
        const std::size_t objectsToVisit                              = std::min(maxObjectsPerPoll, objects.size());

        for (std::size_t visited = 0; visited < objectsToVisit; ++visited)
        {
            if (m_sweepCursor >= objects.size())
            {
                m_sweepCursor = 0;
            }

            const std::shared_ptr<sbk::core::object>& object = objects[m_sweepCursor++];

            if (!object)
            {
                continue;
            }

            sbk::core::database_object* const databaseObject =
                object->try_convert_object<sbk::core::database_object>();

            if (databaseObject == nullptr || databaseObject->get_database_id() == SBK_INVALID_ID)
            {
                continue;
            }

            m_scratchValues.clear();
            databaseObject->get_synced_property_values(m_scratchValues);

            if (m_scratchValues.empty())
            {
                continue;
            }

            std::unordered_map<std::uint32_t, float>& shadow = m_lastSentValues[databaseObject->get_database_id()];

            for (const auto& [propertyID, value] : m_scratchValues)
            {
                const auto previous = shadow.find(propertyID);

                // An unseen property sends its current value - that is the
                // initial sync covering edits made before connecting.
                if (previous == shadow.end() || previous->second != value)
                {
                    if (session.send_set_property(databaseObject->get_database_id(), propertyID, value).has_value())
                    {
                        shadow[propertyID] = value;
                    }
                }
            }
        }
    }
}  // namespace sbk::engine::profiling
