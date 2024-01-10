#pragma once

#include "miniaudio.h"

#include "sound_bakery/core/core_include.h"

namespace SB::Engine
{
    class Sound : public SB::Core::DatabaseObject
    {
    public:
        Sound();
        ~Sound();

    public:
        void loadSynchronous();
        void loadAsynchronous();
        void setSoundName(const std::string& soundName);
        void setStreaming(bool streaming) { m_streaming = streaming; }

        const std::string& getSoundName() const;
        bool getIsStreaming() const { return m_streaming; }

        ma_sound* getSound()
        {
            return &m_sound;
        }

    private:
        ma_sound m_sound;

    private:
        std::string m_soundName;
        bool m_streaming;

        RTTR_ENABLE(SB::Core::DatabaseObject)
        RTTR_REGISTRATION_FRIEND
    };
}