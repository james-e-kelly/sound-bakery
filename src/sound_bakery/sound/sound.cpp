#include "sound_bakery/sound/sound.h"

using namespace SB::Engine;

DEFINE_REFLECTION(SB::Engine::Sound)

Sound::Sound() : m_soundName(), m_sound(), m_streaming(false) {}

void Sound::loadSynchronous()
{
    if (m_soundName.size())
    {
        sc_sound* loadedSound = nullptr;

        sc_result result = sc_system_create_sound(getChef(), m_soundName.c_str(), SC_SOUND_MODE_DEFAULT, &loadedSound);
        assert(result == MA_SUCCESS);

        m_sound.reset(loadedSound);
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