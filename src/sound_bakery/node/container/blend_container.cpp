#include "blend_container.h"

DEFINE_REFLECTION(sbk::engine::BlendContainer)

void sbk::engine::BlendContainer::gatherChildrenForPlay(GatherChildrenContext& context) const
{
    for (NodeBase* const child : getChildren())
    {
        if (child != nullptr)
        {
            if (Container* const childContainer = child->try_convert_object<Container>())
            {
                context.sounds.push_back(childContainer);
            }
        }
    }
}
