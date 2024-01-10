#pragma once

#include "container.h"

namespace SB::Engine
{
    class Sound;

    class SoundContainer : public Container
    {
    public:
        virtual void gatherSounds(std::vector<Container*>& soundContainers) override;

        Sound* getSound();

        void setSound(const SB::Core::DatabasePtr<SB::Engine::Sound>& sound);

    private:
        SB::Core::DatabasePtr<SB::Engine::Sound> m_sound;

        RTTR_ENABLE(Container)
        RTTR_REGISTRATION_FRIEND
    };
}