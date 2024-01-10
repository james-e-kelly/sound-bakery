#pragma once

#include "sound_bakery/node/node.h"

namespace SB::Engine
{
    /**
     * @brief Base container type. Inherited types include sounds, random, sequence etc.
    */
    class Container : public Node
    {
    public:
        Container()
            : Node()
        {
        }

        virtual void onLoaded() override;
        virtual void gatherSounds(std::vector<Container*>& soundContainers) {}

        RTTR_ENABLE(Node)
        RTTR_REGISTRATION_FRIEND
    };
}