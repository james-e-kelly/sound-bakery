#pragma once

#include "sound_bakery/node/node.h"
#include "sound_bakery/parameter/parameter.h"

namespace SB::Engine
{

    /**
     * @brief Base container type. Inherited types include sounds, random,
     * sequence etc.
     */
    class Container : public Node
    {
    public:
        using RuntimeFloatParameterMap =
            std::unordered_map<SB_ID,
                               SB::Engine::FloatParameter::RuntimeProperty>;
        using RuntimeIntParameterMap =
            std::unordered_map<SB_ID,
                               SB::Engine::IntParameter::RuntimeProperty>;

    public:
        Container() : Node() {}

        virtual void onLoaded() override;
        virtual void gatherSounds(
            std::vector<Container*>& soundContainers,
            const RuntimeFloatParameterMap& runtimeFloatParameters,
            const RuntimeIntParameterMap& runtimeIntParameters)
        {
        }

        RTTR_ENABLE(Node)
        RTTR_REGISTRATION_FRIEND
    };
}  // namespace SB::Engine