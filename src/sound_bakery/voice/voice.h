#pragma once

#include "sound_bakery/core/object/object.h"

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
        /**
         * @brief A voice handle -> the container that started it.
         * 
         * When the handle ends, we can retrigger the container to see if it wants any more sounds to play.
         */
        using play_pair = std::pair<sc_voice_handle, std::shared_ptr<container>>;

        auto play_container(container* container) -> sbk::result<void>;

        auto update() -> void;

        [[nodiscard]] auto playing_container(container* container) const noexcept -> bool;
        [[nodiscard]] auto is_playing() const -> bool;
        [[nodiscard]] auto get_owning_game_object() const -> game_object*;

    private:
        struct container_instance
        {
            sbk_id containerReference{};    //< ID to the original container this was played from
            sc_voice_handle voiceHandle{};  //< Valid if this instance is a sound container
            unsigned int childCount{};      //< How many children this container played and waiting to finish
            std::size_t parentIndex{};      //< Array index of our parent so we can quickly find it without searching the adjadcecy array
            bool finished{};                //< Set to true when we've already notified the parent we've ended
        };

        eastl::vector<container_instance> m_instances;
        eastl::vector<std::shared_ptr<sc_node_group>> m_outputBusses;
    };
}  // namespace sbk::engine