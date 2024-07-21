#pragma once

#include "container.h"

namespace sbk::engine
{
    class sound;

    class SB_CLASS SoundContainer : public Container
    {
    public:
        SoundContainer() = default;

        void gatherChildrenForPlay(GatherChildrenContext& context) const override;

        bool can_add_children() const override { return false; }
        bool can_add_child_type(const rttr::type& childType) const override { return false; }

        bool can_add_parent_type(const rttr::type& parentType) const override;

        sound* getSound() const;

        void setSound(const sbk::core::database_ptr<sbk::engine::sound>& sound);

    private:
        sbk::core::database_ptr<sbk::engine::sound> m_sound;

        REGISTER_REFLECTION(SoundContainer, Container)
    };
}  // namespace sbk::engine