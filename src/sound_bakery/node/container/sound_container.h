#pragma once

#include "container.h"

namespace sbk::engine
{
    class sound;

    class SB_CLASS sound_container : public container
    {
    public:
        sound_container() = default;

        auto gather_children_for_play(gather_children_context& context) const -> void override;

        [[nodiscard]] auto can_add_children() const -> bool override { return false; }
        [[nodiscard]] auto can_add_child_type(const rttr::type& childType) const -> bool override { return false; }

        [[nodiscard]] auto can_add_parent_type(const rttr::type& parentType) const -> bool override;

        [[nodiscard]] auto get_sound() const -> sound*;
        auto set_sound(const sbk::core::database_ptr<sbk::engine::sound>& sound) -> void;

    private:
        sbk::core::database_ptr<sbk::engine::sound> m_sound;

        REGISTER_REFLECTION(sound_container, container)
    };
}  // namespace sbk::engine