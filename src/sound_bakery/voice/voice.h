#pragma once

#include "sound_bakery/core/object/object.h"

namespace sbk::engine
{
    class container;
    class game_object;

    /**
     * @brief A runtime graph of nodes and busses, playing a sound or many.
     */
    class SB_CLASS voice : public sbk::core::object
    {
        REGISTER_REFLECTION(voice, sbk::core::object)

    public:
        /**
         * @brief A voice handle -> the container that started it.
         *
         * When the handle ends, we can retrigger the container to see if it wants any more sounds to play.
         */
        using play_pair = std::pair<sc_voice_handle, std::shared_ptr<container>>;

        struct container_instance
        {
            sbk_id containerReference{};
            sc_voice_handle voiceHandle{};
            unsigned int childCount{};
            std::size_t parentIndex{};
            bool finished{};
        };

        auto play_container(container* container) -> sbk::result<void>;

        auto update() -> void;

        [[nodiscard]] auto playing_container(container* container) const noexcept -> bool;
        [[nodiscard]] auto is_playing() const -> bool;
        [[nodiscard]] auto get_owning_game_object() const -> game_object*;

        [[nodiscard]] auto get_instances() const -> const eastl::vector<container_instance>& { return m_instances; }
        [[nodiscard]] auto get_output_busses() const -> const eastl::vector<std::shared_ptr<sc_node_group>>& { return m_outputBusses; }

    private:
        eastl::vector<container_instance> m_instances;
        eastl::vector<std::shared_ptr<sc_node_group>> m_outputBusses;
    };
}  // namespace sbk::engine