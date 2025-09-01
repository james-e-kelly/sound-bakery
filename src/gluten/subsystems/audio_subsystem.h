#pragma once

#include "gluten/subsystems/subsystem.h"

#include "sound_chef/sound_chef.h"

namespace gluten
{
	/**
	 * @brief A simple audio subsystem that can play audio using Sound Chef.
     * 
     * The audio_subsystem does not use Sound Bakery as there is no need for adaptive audio. 
     * Instead, the subsystem acts as a normal application audio player, suitable for playing audio files
     * or reading wav data.
	 */
	class audio_subsystem : public subsystem
	{
    public:
        audio_subsystem(app* appOwner) : subsystem(appOwner) {}

        int init() override;

        auto play_sound(const std::filesystem::path& filePath) -> void;
        auto pause_sound(const std::filesystem::path& filePath) -> void;
        auto get_sound_cursor_position(const std::filesystem::path& filePath) -> float;
        auto get_sound_length(const std::filesystem::path& filePath) -> float;

        auto get_or_load_audio_handle(const std::filesystem::path& filePath) -> sc_sound*;
        auto get_sound_instance(const std::filesystem::path& filePath) -> sc_sound_instance*;

    protected:
        struct sc_system_deleter
        {
            auto operator()(sc_system* system) -> void;
        };

        struct sc_sound_deleter
        {
            void operator()(sc_sound* sound) { sc_sound_release(sound); }
        };

        struct sc_sound_instance_deleter
        {
            void operator()(sc_sound_instance* soundInstance) { sc_sound_instance_release(soundInstance); }
        };

        std::unique_ptr<sc_system, sc_system_deleter> m_soundChef;
        std::unordered_map<std::filesystem::path, std::unique_ptr<sc_sound, sc_sound_deleter>> m_filesToSoundsMap;
        std::unordered_map<std::filesystem::path, std::unique_ptr<sc_sound_instance, sc_sound_deleter>> m_filesToSoundInstancesMap;
	};
}