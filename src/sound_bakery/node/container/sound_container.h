#pragma once

#include "container.h"

namespace sbk::engine
{
    class Sound;

    class SB_CLASS SoundContainer : public Container
    {
    public:
        SoundContainer() = default;

        void gatherChildrenForPlay(GatherChildrenContext& context) const override;

        bool canAddChild(const sbk::core::database_ptr<NodeBase>& child) const override { return false; }

        Sound* getSound() const;

        void setSound(const sbk::core::database_ptr<sbk::engine::Sound>& sound);

    private:
        sbk::core::database_ptr<sbk::engine::Sound> m_sound;

        REGISTER_REFLECTION(SoundContainer, Container)
    };
}  // namespace sbk::engine