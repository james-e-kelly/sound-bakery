#pragma once

#include "sound_bakery/pch.h"

namespace SB::Engine
{
    class System;

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
            void update(System* system);

        public:
            unsigned int getPlayingCountOfObject(SB_ID id) const;

        private:
            std::unordered_set<SB_ID> m_playingNodeIDs;
            std::unordered_map<SB_ID, unsigned int> m_nodePlayingCount;
        };
    }  // namespace Profiling
}  // namespace SB::Engine