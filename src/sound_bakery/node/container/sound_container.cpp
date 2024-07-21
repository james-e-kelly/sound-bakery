#include "sound_container.h"

#include "sound_bakery/sound/sound.h"

DEFINE_REFLECTION(sbk::engine::SoundContainer)

void sbk::engine::SoundContainer::gather_children_for_play(GatherChildrenContext& context) const
{
    context.sounds.push_back(this);
}

bool sbk::engine::SoundContainer::can_add_parent_type(const rttr::type& parentType) const
{
    return sbk::engine::node_base::can_add_parent_type(parentType) && parentType != sbk::engine::SoundContainer::type();
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
