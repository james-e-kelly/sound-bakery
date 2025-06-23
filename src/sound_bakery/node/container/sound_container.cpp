#include "sound_container.h"

#include "sound_bakery/sound/sound.h"

DEFINE_REFLECTION(sbk::engine::sound_container)

void sbk::engine::sound_container::gather_children_for_play(gather_children_context& context) const
{
    context.sounds.push_back(const_cast<sbk::engine::sound_container*>(this));
}

bool sbk::engine::sound_container::can_add_parent_type(const rttr::type& parentType) const
{
    return sbk::engine::node_base::can_add_parent_type(parentType) &&
           parentType != sbk::engine::sound_container::type();
}

sbk::engine::sound* sbk::engine::sound_container::get_sound() const
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

void sbk::engine::sound_container::set_sound(const sbk::core::database_ptr<sbk::engine::sound>& sound)
{
    m_sound = sound;
}
