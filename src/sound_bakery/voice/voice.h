#pragma once

#include "sound_bakery/core/core_include.h"
#include "sound_bakery/voice/node_instance.h"

namespace SB::Engine
{
    class Container;
    class GameObject;
    class NodeInstance;

    /**
     * @brief A runtime graph of nodes and busses, playing a sound or many.
     *
     */
    class SB_CLASS Voice : public SB::Core::object
    {
    public:
        Voice() = delete;
        Voice(GameObject* owningObject) : m_owningGameObject(owningObject) {}

    public:
        void playContainer(Container* container);
        void update();

    public:
        bool playingContainer(Container* container) const noexcept;

        const std::vector<std::unique_ptr<NodeInstance>>& getVoices() const noexcept;

        std::size_t voices() const;

        NodeInstance* voice(std::size_t index) const;

        bool isPlaying() const;

        GameObject* getOwningGameObject() const { return m_owningGameObject; }

    private:
        GameObject* m_owningGameObject = nullptr;
        SB::Core::DatabasePtr<Container> m_playingContainer;

        std::vector<std::unique_ptr<NodeInstance>> m_voiceInstances;
    };
}  // namespace SB::Engine