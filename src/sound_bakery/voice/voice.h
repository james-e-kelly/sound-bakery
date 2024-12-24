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
        void play_container(container* container);

        void update();

        [[nodiscard]] bool playing_container(container* container) const noexcept;

        [[nodiscard]] const std::vector<std::shared_ptr<node_instance>> get_voices() const noexcept;
        [[nodiscard]] std::size_t num_voices() const;
        [[nodiscard]] node_instance* node_instance_at(std::size_t index) const;

        [[nodiscard]] bool is_playing() const;

        game_object* get_owning_game_object() const;

    private:
        sbk::core::database_ptr<container> m_playingContainer;
    };
}  // namespace sbk::engine