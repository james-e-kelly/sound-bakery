#include "sound_container.h"

#include "sound_bakery/sound/sound.h"

DEFINE_REFLECTION(sbk::engine::sound_container)

auto sbk::engine::sound_container::gather_children_for_play(gather_children_context& context) const -> void
{
    ZoneScoped;
    context.sounds.push_back(sbk::core::database_ptr<sbk::engine::sound_container>(const_cast<sbk::engine::sound_container*>(this)).shared());
}

auto sbk::engine::sound_container::can_add_parent_type(const rttr::type& parentType) const -> bool
{
    return sbk::engine::node_base::can_add_parent_type(parentType) && parentType != sbk::engine::sound_container::type();
}

auto sbk::engine::sound_container::get_sound() const -> std::shared_ptr<sbk::engine::sound>
{
    return m_sound.shared();
}

auto sbk::engine::sound_container::set_sound(const sbk::core::database_ptr<sbk::engine::sound>& sound) -> void
{
    m_sound = sound;
}
