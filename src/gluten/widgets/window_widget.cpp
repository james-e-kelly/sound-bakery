#include "window_widget.h"

auto gluten::window_widget::set_window_flags(ImGuiWindowFlags flags) -> void
{
    m_windowFlags = flags;
}

auto gluten::window_widget::set_window_class(const ImGuiWindowClass& windowClass) -> void
{
    m_windowClass = windowClass;
}

auto gluten::window_widget::render_implementation() -> void
{
    ImGui::SetNextWindowClass(&m_windowClass);

    if (ImGui::Begin(get_widget_name().data(), nullptr, m_windowFlags))
    {
        render_window_implementation();
    }

    ImGui::End();
}