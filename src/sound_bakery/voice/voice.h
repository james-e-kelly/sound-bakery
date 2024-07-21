#pragma once

#include "sound_bakery/core/core_include.h"
#include "sound_bakery/voice/node_instance.h"

namespace sbk::engine
{
    class Container;
    class game_object;
    class node_instance;

    /**
     * @brief A runtime graph of nodes and busses, playing a sound or many.
     */
    class SB_CLASS voice : public sbk::core::object
    {
    public:
        voice() = delete;
        voice(game_object* owningObject) : m_owningGameObject(owningObject) {}

        void play_container(Container* container);

        void update();

        [[nodiscard]] bool playing_container(Container* container) const noexcept;

        [[nodiscard]] const std::vector<std::unique_ptr<node_instance>>& get_voices() const noexcept;
        [[nodiscard]] std::size_t num_voices() const;
        [[nodiscard]] node_instance* node_instance_at(std::size_t index) const;

        [[nodiscard]] bool is_playing() const;

        game_object* get_owning_game_object() const { return m_owningGameObject; }

    private:
        game_object* m_owningGameObject = nullptr;
        sbk::core::database_ptr<Container> m_playingContainer;

        std::vector<std::unique_ptr<node_instance>> m_voiceInstances;
    };
}  // namespace sbk::engine