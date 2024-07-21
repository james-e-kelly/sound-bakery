#pragma once

#include "sound_bakery/core/core_include.h"
#include "sound_bakery/voice/node_instance.h"

namespace sbk::engine
{
    class Container;
    class game_object;
    class NodeInstance;

    /**
     * @brief A runtime graph of nodes and busses, playing a sound or many.
     *
     */
    class SB_CLASS voice : public sbk::core::object
    {
    public:
        voice() = delete;
        voice(game_object* owningObject) : m_owningGameObject(owningObject) {}

    public:
        void playContainer(Container* container);
        void update();

    public:
        bool playingContainer(Container* container) const noexcept;

        const std::vector<std::unique_ptr<NodeInstance>>& getVoices() const noexcept;

        std::size_t voices() const;

        NodeInstance* node_instance_at(std::size_t index) const;

        bool isPlaying() const;

        game_object* getOwningGameObject() const { return m_owningGameObject; }

    private:
        game_object* m_owningGameObject = nullptr;
        sbk::core::database_ptr<Container> m_playingContainer;

        std::vector<std::unique_ptr<NodeInstance>> m_voiceInstances;
    };
}  // namespace sbk::engine