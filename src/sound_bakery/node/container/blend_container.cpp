#include "blend_container.h"

DEFINE_REFLECTION(sbk::engine::BlendContainer)

void sbk::engine::BlendContainer::gather_children_for_play(GatherChildrenContext& context) const
{
    for (node_base* const child : getChildren())
    {
        if (child != nullptr)
        {
            if (container* const childContainer = child->try_convert_object<container>())
            {
                context.sounds.push_back(childContainer);
            }
        }
    }
}
