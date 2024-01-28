#include "blend_container.h"

void SB::Engine::BlendContainer::gatherSounds(GatherSoundsContext& context)
{
    for (NodeBase* const child : getChildren())
    {
        if (child != nullptr)
        {
            if (Container* const childContainer = child->tryConvertObject<Container>())
            {
                childContainer->gatherSounds(context);
            }
        }
    }
}
