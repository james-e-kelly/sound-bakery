#pragma once

#include "sound_bakery/pch.h"

namespace sbk::engine
{
    class system;

    namespace profiling
    {
        /**
         * @brief Tracks every active voice with helper functions
         */
        class SB_CLASS voice_tracker final
        {
        public:
            voice_tracker()  = default;
            ~voice_tracker() = default;

        public:
            auto update(system* system) -> void;

        public:
            [[nodiscard]] auto get_playing_count_of_object(sbk_id id) const -> unsigned int;

        private:
            std::unordered_set<sbk_id> m_playingNodeIDs;
            std::unordered_map<sbk_id, unsigned int> m_nodePlayingCount;
        };
    }  // namespace profiling
}  // namespace sbk::engine