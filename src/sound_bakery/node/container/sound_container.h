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

        bool canAddChild(const sbk::core::database_ptr<node_base>& child) const override { return false; }

        sound* getSound() const;

        void setSound(const sbk::core::database_ptr<sbk::engine::sound>& sound);

    private:
        sbk::core::database_ptr<sbk::engine::sound> m_sound;

        REGISTER_REFLECTION(SoundContainer, Container)
    };
}  // namespace sbk::engine