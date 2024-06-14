#pragma once

#include "container.h"

namespace SB::Engine
{
    class Sound;

    class SB_CLASS SoundContainer : public Container
    {
    public:
        SoundContainer() = default;

        void gatherChildrenForPlay(GatherChildrenContext& context) const override;

        bool canAddChild(const SB::Core::DatabasePtr<NodeBase>& child) const override { return false; }

        Sound* getSound() const;

        void setSound(const SB::Core::DatabasePtr<SB::Engine::Sound>& sound);

    private:
        SB::Core::DatabasePtr<SB::Engine::Sound> m_sound;

        REGISTER_REFLECTION(SoundContainer, Container)
    };
}  // namespace SB::Engine