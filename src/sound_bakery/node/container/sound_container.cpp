#include "sound_container.h"

#include "sound_bakery/sound/sound.h"

DEFINE_REFLECTION(sbk::engine::SoundContainer)

void sbk::engine::SoundContainer::gatherChildrenForPlay(GatherChildrenContext& context) const
{
    context.sounds.push_back(this);
}

sbk::engine::sound* sbk::engine::SoundContainer::getSound() const
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

void sbk::engine::SoundContainer::setSound(const sbk::core::database_ptr<sbk::engine::sound>& sound)
{
    m_sound = sound;
}
