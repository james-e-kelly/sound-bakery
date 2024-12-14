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
            void update(system* system);

        public:
            unsigned int getPlayingCountOfObject(sbk_id id) const;

        private:
            std::unordered_set<sbk_id> m_playingNodeIDs;
            std::unordered_map<sbk_id, unsigned int> m_nodePlayingCount;
        };
    }  // namespace profiling
}  // namespace sbk::engine