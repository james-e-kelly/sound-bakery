#pragma once

#include "sound_bakery/pch.h"

namespace sbk::engine
{
    class system;

    namespace Profiling
    {
        /**
         * @brief Tracks every active voice with helper functions
         */
        class SB_CLASS VoiceTracker final
        {
        public:
            VoiceTracker();
            ~VoiceTracker() = default;

            static VoiceTracker* get();

        public:
            void update(system* system);

        public:
            unsigned int getPlayingCountOfObject(sbk_id id) const;

        private:
            std::unordered_set<sbk_id> m_playingNodeIDs;
            std::unordered_map<sbk_id, unsigned int> m_nodePlayingCount;
        };
    }  // namespace Profiling
}  // namespace sbk::engine