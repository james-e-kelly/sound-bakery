#pragma once

#include "sound_bakery/node/node.h"

namespace SB::Engine
{
    class Container;

    /**
     * @brief Contains all information required for gathering sounds for runtime playing and selection.
     */
    struct SB_CLASS GatherChildrenContext
    {
        GatherChildrenContext()
        {
            // We're making the rough assumption that each call to gatherChildrenForPlay
            // will gather, on average, 3 or less sounds.
            // This should hopefully save some allocation time in most cases.
            sounds.reserve(3);
        }

        /**
         * @brief Vector of containers that should play this iteration.
         */
        std::vector<const Container*> sounds;

        /**
         * @brief List of parameters that are local to this gathering.
         *
         * Use this to choose sounds based on the current/local context.
         */
        LocalParameterList parameters;

        /**
         * @brief How many times the voice/node has played.
         *
         * Will equal 1 in most cases.
         *
         * For sequences, this can be higher.
         */
        unsigned int numTimesPlayed;
    };

    /**
     * @brief Base container type. Inherited types include sounds, random,
     * sequence etc.
     */
    class SB_CLASS Container : public Node
    {
    public:
        Container() : Node() {}

        void onLoaded() override;

        bool canAddChild(const SB::Core::DatabasePtr<NodeBase>& child) const override
        {
            if (child.lookup() && child->getType().is_derived_from<Container>())
            {
                return NodeBase::canAddChild(child);
            }
            else
            {
                return false;
            }
        }

        /**
         * @brief Collects and gathers sounds on this node and its children for play.
         *
         * Given a context object with parameters relevant to the local reference, chooses which sounds/containers
         * to play.
         *
         * @param context for this gather sounds call.
         */
        virtual void gatherChildrenForPlay(GatherChildrenContext& context) const = 0;

        REGISTER_REFLECTION(Container, Node)
        RTTR_REGISTRATION_FRIEND
    };
}  // namespace SB::Engine