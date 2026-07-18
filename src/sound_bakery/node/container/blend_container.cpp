#include "blend_container.h"

DEFINE_REFLECTION(sbk::engine::blend_container)

auto sbk::engine::blend_container::gather_children_for_play(gather_children_context& context) const -> void
{
    for (node_base* const child : get_children())
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
