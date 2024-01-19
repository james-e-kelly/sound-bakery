#pragma once

#include "sound_bakery/core/core_include.h"

namespace SB::Engine
{
    class Container;
    class Event;
    class Voice;

    class GameObject : public SB::Core::Object
    {
    public:
        GameObject();
        ~GameObject();

    public:
        Voice* playContainer(Container* container);
        void postEvent(Event* event);

        void stopVoice(Voice* voice);
        void stopContainer(Container* container);
        void stopAll();

    public:
        void update();

    public:
        bool isPlaying() const noexcept;

        std::size_t voiceCount() const noexcept;

        Voice* getVoice(std::size_t index) const;

    private:
        std::vector<std::unique_ptr<Voice>> m_voices;
    };
}  // namespace SB::Engine