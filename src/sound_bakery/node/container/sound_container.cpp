#include "sound_container.h"

#include "sound_bakery/sound/sound.h"

void SB::Engine::SoundContainer::gatherSounds(std::vector<Container*>& soundContainers)
{
    soundContainers.push_back(this);
}

SB::Engine::Sound* SB::Engine::SoundContainer::getSound()
{
    if (m_sound.lookup())
    {
        return m_sound.raw();
    }
    else
    {
        return nullptr;
    }
}

void SB::Engine::SoundContainer::setSound(const SB::Core::DatabasePtr<SB::Engine::Sound>& sound)
{
    m_sound = sound;
}
