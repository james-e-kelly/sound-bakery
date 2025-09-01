#include "audio_subsystem.h"

#define CHECK_SC_RESULT(result)     \
    if (result != SBK_SUCCESS)      \
    {                               \
        return 1;                   \
    }

auto gluten::audio_subsystem::sc_system_deleter::operator()(sc_system* system) -> void
{
    if (system)
    {
        sc_system_close(system);
        sc_system_release(system);
    }
}

int gluten::audio_subsystem::init()
{
    sc_system* system = nullptr;
    sbk_result result = sc_system_create(&system);
    CHECK_SC_RESULT(result);

    m_soundChef.reset(system);

    if (m_soundChef)
    {
        const sc_system_config config = sc_system_config_init_default();
        result = sc_system_init(m_soundChef.get(), &config);
        CHECK_SC_RESULT(result);
    }

    return 0;
}

auto gluten::audio_subsystem::play_sound(const std::filesystem::path& filePath) -> void
{
    if (sc_sound_instance* const soundInstance = get_sound_instance(filePath))
    {
        sc_bool playing = MA_FALSE;
        sc_sound_instance_is_playing(soundInstance, &playing);

        if (!playing)
        {
            sc_sound_instance_start(soundInstance);
        }
    }
    else if (sc_sound* const sound = get_or_load_audio_handle(filePath))
    {
        sc_sound_instance* soundInstance = nullptr;
        sc_system_play_sound(m_soundChef.get(), sound, &soundInstance, nullptr, SBK_FALSE);
        m_filesToSoundInstancesMap[filePath].reset(soundInstance);
    }
}

auto gluten::audio_subsystem::pause_sound(const std::filesystem::path& filePath) -> void
{
    if (sc_sound_instance* const soundInstance = get_sound_instance(filePath))
    {
        sc_bool playing = MA_FALSE;
        sc_sound_instance_is_playing(soundInstance, &playing);

        if (playing)
        {
            sc_sound_instance_pause(soundInstance);
        }
    }
}

auto gluten::audio_subsystem::get_sound_cursor_position(const std::filesystem::path& filePath) -> float
{
    float seconds = 0.0f;

    if (sc_sound_instance* const soundInstance = get_sound_instance(filePath))
    {
        sc_sound_instance_get_cursor_in_seconds(soundInstance, &seconds);
    }

    return seconds;
}

auto gluten::audio_subsystem::get_sound_length(const std::filesystem::path& filePath) -> float
{
    float seconds = 0.0f;

   if (sc_sound* const sound = get_or_load_audio_handle(filePath))
    {
        sc_sound_get_length(sound, &seconds);
    }

    return seconds;
}

auto gluten::audio_subsystem::get_or_load_audio_handle(const std::filesystem::path& filePath) -> sc_sound*
{
    sc_sound* sound = nullptr;

    if (std::filesystem::exists(filePath))
    {
        if (m_filesToSoundsMap.contains(filePath))
        {
            sound = m_filesToSoundsMap.at(filePath).get();
        }
        else if (sc_system_create_sound(m_soundChef.get(), filePath.string().c_str(), SC_SOUND_MODE_DECODE, &sound) == SBK_SUCCESS)
        {
            m_filesToSoundsMap[filePath].reset(sound);
        }
    }
    return sound;
}

auto gluten::audio_subsystem::get_sound_instance(const std::filesystem::path& filePath) -> sc_sound_instance*
{
    sc_sound_instance* soundInstance = nullptr;

    if (std::filesystem::exists(filePath))
    {
        if (m_filesToSoundInstancesMap.contains(filePath))
        {
            soundInstance = m_filesToSoundInstancesMap.at(filePath).get();
        }
    }
    return soundInstance;
}