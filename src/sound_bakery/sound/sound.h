#pragma once

#include "sound_bakery/core/core_include.h"
#include "sound_chef/sound_chef.h"
#include "sound_chef/sound_chef_encoder.h"

#include <boost/serialization/binary_object.hpp>

namespace sbk::engine
{
    /**
     * @brief Data for converting a sound to an encoded format.
     */
    struct SB_CLASS encoding_sound
    {
        std::filesystem::path rawSoundPath;
        std::filesystem::path encodedSoundPath;
        sc_encoding_format encodingFormat = sc_encoding_format_vorbis;
    };

    struct void_deleter
    {
        void operator()(void* data)
        {
            if (data)
            {
                std::free(data);
            }
        }
    };

    using raw_sound_ptr = std::unique_ptr<void, void_deleter>;

    class SB_CLASS sound : public sbk::core::database_object
    {
        REGISTER_REFLECTION(sound, sbk::core::database_object)
        LEAK_DETECTOR(sound)

    public:
        auto load_synchronous() -> void;
        auto load_asynchronous() -> void;

        auto set_sound_name(std::string soundName) -> void;
        auto set_encoded_sound_name(std::string path) -> void;
        auto get_encoded_sound_name() const -> std::string { return encodedSoundPath.string(); }
        auto set_is_streaming(bool streaming) -> void { m_streaming = streaming; }
        auto set_raw_sound_data(raw_sound_ptr& data) -> void
        {  
            m_memorySoundData = std::move(data);
        }

        auto get_sound_name() const -> std::string;
        auto get_sound() -> sc_sound*;
        auto get_is_streaming() const -> bool{ return m_streaming; }
        auto get_encoding_sound_data() const -> encoding_sound;

    private:
        std::unique_ptr<sc_sound, SC_SOUND_DELETER> m_sound;
        raw_sound_ptr m_memorySoundData;

        std::filesystem::path rawSoundPath;
        std::filesystem::path encodedSoundPath;

        sc_encoding_format m_encodingFormat = sc_encoding_format_vorbis;

        bool m_streaming = false;

    };
}  // namespace sbk::engine