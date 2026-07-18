#include "blend_container.h"

DEFINE_REFLECTION(sbk::engine::blend_container)

auto sbk::engine::blend_container::gather_children_for_play(gather_children_context& context) const -> void
{
    for (const auto& child : get_children())
    {
        if (child)
        {
            if (const auto childContainer = std::static_pointer_cast<container>(child))
            {
                context.sounds.push_back(childContainer);
            }
        }
    }
}
