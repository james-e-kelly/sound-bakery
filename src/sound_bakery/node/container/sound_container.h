#pragma once

#include "container.h"

namespace sbk::engine
{
    class sound;

    class SB_CLASS sound_container : public container
    {
    public:
        sound_container() = default;

        void gather_children_for_play(gather_children_context& context) const override;

        bool can_add_children() const override { return false; }
        bool can_add_child_type(const rttr::type& childType) const override { return false; }

        bool can_add_parent_type(const rttr::type& parentType) const override;

        [[nodiscard]] sound* get_sound() const;
        void set_sound(const sbk::core::database_ptr<sbk::engine::sound>& sound);

    private:
        sbk::core::database_ptr<sbk::engine::sound> m_sound;

        REGISTER_REFLECTION(sound_container, container)
    };
}  // namespace sbk::engine