#include "window_widget.h"

#include "gluten/utils/imgui_util_structures.h"

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

    gluten::imgui::scoped_style noWindowPadding(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    if (ImGui::Begin(get_widget_name().data(), nullptr, m_windowFlags))
    {
        render_window_implementation();
    }

    ImGui::End();
}