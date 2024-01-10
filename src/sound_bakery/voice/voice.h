#pragma once

#include "sound_bakery/core/core_include.h"

namespace SB::Engine
{
    class Container;
    class NodeInstance;

    /**
     * @brief A runtime graph of nodes and busses, playing a sound or many.
     * 
     */
    class Voice : public SB::Core::Object
    {
    public:
        Voice();
        ~Voice();

    public:
        void playContainer(Container* container) noexcept;
        void update();

    public:
        bool playingContainer(Container* container) const noexcept;

        const std::vector<std::unique_ptr<NodeInstance>>& getVoices() const noexcept;

        std::size_t voices() const;

        NodeInstance* voice(std::size_t index) const;

        bool isPlaying() const;

    private:
        SB::Core::DatabasePtr<Container> m_playingContainer;

        std::vector<std::unique_ptr<NodeInstance>> m_voiceInstances;
    };
}