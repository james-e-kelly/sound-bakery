#pragma once

#include "miniaudio.h"
#include "sound_bakery/core/core_include.h"

namespace SB::Engine
{
    class SB_CLASS Sound : public SB::Core::database_object
    {
    public:
        Sound();

        void loadSynchronous();
        void loadAsynchronous();

        void setSoundName(std::string soundName);
        std::string getSoundName() const;

        void setEncodedSoundName(std::string path);
        std::string getEncodedSoundName() const { return encodedSoundPath.string(); }

        void setStreaming(bool streaming) { m_streaming = streaming; }
        bool getIsStreaming() const { return m_streaming; }

        sc_sound* getSound();

    private:
        std::unique_ptr<sc_sound, SC_SOUND_DELETER> m_sound;

        std::filesystem::path rawSoundPath;
        std::filesystem::path encodedSoundPath;

        bool m_streaming;

        REGISTER_REFLECTION(Sound, SB::Core::database_object)
        RTTR_REGISTRATION_FRIEND
    };
}  // namespace SB::Engine