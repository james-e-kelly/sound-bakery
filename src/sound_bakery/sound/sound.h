#pragma once

#include "miniaudio.h"
#include "sound_bakery/core/core_include.h"

namespace SB::Engine
{
    class SB_CLASS Sound : public SB::Core::DatabaseObject
    {
    public:
        Sound();

        void loadSynchronous();
        void loadAsynchronous();

        void setSoundName(const std::string& soundName);
        const std::string& getSoundName() const;

        void setStreaming(bool streaming) { m_streaming = streaming; }
        bool getIsStreaming() const { return m_streaming; }

        SC_SOUND* getSound() { return m_sound.get(); }

    private:
        std::unique_ptr<SC_SOUND, SC_SOUND_DELETER> m_sound;
        std::string m_soundName;
        bool m_streaming;

        REGISTER_REFLECTION(Sound, SB::Core::DatabaseObject)
        RTTR_REGISTRATION_FRIEND
    };
}  // namespace SB::Engine