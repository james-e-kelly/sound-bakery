#pragma once

#include "sound_chef/sound_chef.h"
#include "sound_chef/sound_chef_encoder.h"
#include "sound_bakery/core/core_include.h"

namespace sbk::engine
{
    /**
     * @brief Data for converting a sound to an encoded format.
    */
    struct SB_CLASS encoding_sound
    {
        std::filesystem::path rawSoundPath;
        std::filesystem::path encodedSoundPath;
        sc_encoding_format encodingFormat = sc_encoding_format_wav;
    };

    class SB_CLASS sound : public sbk::core::database_object
    {
    public:
        sound();

        void loadSynchronous();
        void loadAsynchronous();

        void setSoundName(std::string soundName);
        std::string getSoundName() const;

        void setEncodedSoundName(std::string path);
        std::string getEncodedSoundName() const { return encodedSoundPath.string(); }

        void setStreaming(bool streaming) { m_streaming = streaming; }
        bool getIsStreaming() const { return m_streaming; }

        sc_sound* getSound();

        encoding_sound get_encoding_sound_data() const;

    private:
        std::unique_ptr<sc_sound, SC_SOUND_DELETER> m_sound;

        std::filesystem::path rawSoundPath;
        std::filesystem::path encodedSoundPath;

        sc_encoding_format m_encodingFormat = sc_encoding_format_wav;

        bool m_streaming = false;

        REGISTER_REFLECTION(sound, sbk::core::database_object)
        RTTR_REGISTRATION_FRIEND
    };
}  // namespace sbk::engine