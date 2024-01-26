#include "blend_container.h"

void SB::Engine::BlendContainer::gatherSounds(std::vector<Container*>& soundContainers,
                                              const RuntimeFloatParameterMap& runtimeFloatParameters,
                                              const RuntimeIntParameterMap& runtimeIntParameters)
{
    for (NodeBase* const child : getChildren())
    {
        if (child)
        {
            if (Container* const childContainer = rttr::rttr_cast<Container*, NodeBase*>(child))
            {
                childContainer->gatherSounds(soundContainers, runtimeFloatParameters, runtimeIntParameters);
            }
        }
    }
}
