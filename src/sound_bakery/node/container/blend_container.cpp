#include "blend_container.h"

void SB::Engine::BlendContainer::gatherChildrenForPlay(GatherChildrenContext& context) const
{
    for (NodeBase* const child : getChildren())
    {
        if (child != nullptr)
        {
            if (Container* const childContainer = child->tryConvertObject<Container>())
            {
                context.sounds.push_back(childContainer);
            }
        }
    }
}
