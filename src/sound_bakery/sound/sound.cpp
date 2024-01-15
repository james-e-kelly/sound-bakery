#include "sound_bakery/sound/sound.h"

using namespace SB::Engine;

Sound::Sound() : m_soundName(), m_sound(), m_streaming(false) {}

Sound::~Sound() { ma_sound_uninit(&m_sound); }

void Sound::loadSynchronous()
{
    if (m_soundName.size())
    {
        ma_result result = ma_sound_init_from_file(
            getMini(), m_soundName.c_str(), 0, nullptr, nullptr, &m_sound);
        assert(result == MA_SUCCESS);
    }
}

void Sound::loadAsynchronous()
{
    if (m_soundName.size())
    {
        loadSynchronous();
    }
}

void Sound::setSoundName(const std::string& soundName)
{
    m_soundName = soundName;
    loadSynchronous();
}

const std::string& Sound::getSoundName() const { return m_soundName; }