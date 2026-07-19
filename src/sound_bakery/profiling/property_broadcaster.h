#pragma once

#include "sound_bakery/core/database/database_object.h"
#include "sound_bakery/sound_bakery_common.h"

#include <chrono>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace sbk::core
{
    class object_owner;
}  // namespace sbk::core

namespace sbk::engine::profiling
{
    class remote_session;

    /**
     * @brief Automatically broadcasts property edits over a remote connection.
     *
     * The authoring side owns one of these next to its @ref remote_session.
     * Which properties sync is declared in the reflection registration via
     * metadata_key::synced - like Unreal's UPROPERTY(Replicated) - and, also
     * like Unreal, changes are detected by comparing against shadow state
     * rather than by change callbacks. That catches every write path
     * (property::set, reflection set_value, undo, serialization loads) with
     * zero subscription bookkeeping.
     *
     * Cost is bounded for large projects (50k+ objects): the diff runs at
     * @ref pollInterval, sweeps at most @ref maxObjectsPerPoll objects per
     * poll with a round-robin cursor, and per-type reflection lookups are
     * cached by database_object. The initial full sync after connecting is
     * spread across the same sweep instead of bursting every value at once.
     *
     * The first time an object is seen its current values are sent, which is
     * the initial sync: edits made before connecting (and objects created
     * after connecting) reach the runtime automatically. Shadow state drops
     * on disconnect, so a reconnect re-syncs the full state.
     */
    class SB_CLASS property_broadcaster final
    {
    public:
        std::chrono::milliseconds pollInterval{100};  //< How often to diff. 10 Hz feels live for mixing.
        std::size_t maxObjectsPerPoll = 4096;         //< Bounds per-poll cost; a full sweep may span several polls.

        /**
         * @brief Diffs synced properties against shadow state and sends changes. Call once per frame.
         */
        auto update(remote_session& session, sbk::core::object_owner& objectOwner) -> void;

    private:
        std::unordered_map<sbk_id, std::unordered_map<std::uint32_t, float>> m_lastSentValues;
        std::vector<sbk::core::database_object::synced_property_value> m_scratchValues;  //< Reused between objects.
        std::chrono::steady_clock::time_point m_lastPoll{};  //< Epoch start makes the first poll immediate.
        std::size_t m_sweepCursor = 0;
        bool m_wasConnected       = false;
    };
}  // namespace sbk::engine::profiling
