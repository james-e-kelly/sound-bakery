#pragma once

#include "sound_bakery/core/core_include.h"
#include "sound_bakery/voice/node_instance.h"

namespace sbk::engine
{
    class container;
    class game_object;
    class node_instance;

    /**
     * @brief A runtime graph of nodes and busses, playing a sound or many.
     */
    class SB_CLASS voice : public sbk::core::object
    {
        REGISTER_REFLECTION(voice, sbk::core::object)

    public:
        auto play_container(container* container) -> sbk::result<void>;

        auto update() -> void;

        [[nodiscard]] auto playing_container(container* container) const noexcept -> bool;

        [[nodiscard]] auto get_voices() const noexcept -> const std::vector<std::shared_ptr<node_instance>>;
        [[nodiscard]] auto num_voices() const -> std::size_t;
        [[nodiscard]] auto node_instance_at(std::size_t index) const -> node_instance*;

        [[nodiscard]] auto is_playing() const -> bool;

        [[nodiscard]] auto get_owning_game_object() const -> game_object*;

    private:
        sbk::core::database_ptr<container> m_playingContainer;
    };
}  // namespace sbk::engine