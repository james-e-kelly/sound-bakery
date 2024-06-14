#include "sound_container.h"

#include "sound_bakery/sound/sound.h"

DEFINE_REFLECTION(SB::Engine::SoundContainer)

void SB::Engine::SoundContainer::gatherChildrenForPlay(GatherChildrenContext& context) const
{
    context.sounds.push_back(this);
}

SB::Engine::Sound* SB::Engine::SoundContainer::getSound() const
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

void SB::Engine::SoundContainer::setSound(const SB::Core::DatabasePtr<SB::Engine::Sound>& sound) { m_sound = sound; }
