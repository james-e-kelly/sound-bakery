#pragma once

#include "sound_bakery/node/node.h"

namespace sbk::engine
{
    class container;

    /**
     * @brief Contains all information required for gathering sounds for runtime playing and selection.
     */
    struct SB_CLASS gather_children_context
    {
        gather_children_context()
        {
            // We're making the rough assumption that each call to gather_children_for_play
            // will gather, on average, 3 or less sounds.
            // This should hopefully save some allocation time in most cases.
            sounds.reserve(3);
        }

        /**
         * @brief Vector of containers that should play this iteration.
         */
        eastl::vector<std::shared_ptr<container>> sounds;

        /**
         * @brief List of parameters that are local to this gathering.
         *
         * Use this to choose sounds based on the current/local context.
         */
        local_parameter_list parameters;

        /**
         * @brief How many times the voice/node has played.
         *
         * Will equal 1 in most cases.
         *
         * For sequences, this can be higher.
         */
        unsigned int numTimesPlayed = 0;
    };

    /**
     * @brief Base container type. Inherited types include sounds, random,
     * sequence etc.
     */
    class SB_CLASS container : public node
    {
    public:
        container() : node() {}

        [[nodiscard]] virtual auto get_is_export() const -> bool override { return false; }
        [[nodiscard]] auto can_add_child_type(const rttr::type& childType) const -> bool override;

        /**
         * @brief Collects and gathers sounds on this node and its children for play.
         *
         * Given a context object with parameters relevant to the local reference, chooses which sounds/containers
         * to play.
         *
         * @param context for this gather sounds call.
         */
        virtual auto gather_children_for_play(gather_children_context& context) const -> void = 0;

        REGISTER_REFLECTION(container, node)
        RTTR_REGISTRATION_FRIEND
    };
}  // namespace sbk::engine