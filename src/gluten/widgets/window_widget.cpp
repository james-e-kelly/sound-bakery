#include "window_widget.h"

auto gluten::window_widget::render_implementation() -> void
{
    if (ImGui::Begin(get_widget_name().data()))
    {
        render_window_implementation();
    }

    ImGui::End();
}