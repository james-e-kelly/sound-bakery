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

auto gluten::window_widget::set_background_color(ImVec4 color) -> void
{
    m_backgroundColor = color;
}

auto gluten::window_widget::render_implementation() -> void
{
    ImGui::SetNextWindowClass(&m_windowClass);

    gluten::imgui::scoped_color backgroundColoor(ImGuiCol_WindowBg, m_backgroundColor.has_value() ? m_backgroundColor.value() : ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
    gluten::imgui::scoped_style noWindowPadding(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    if (ImGui::Begin(get_widget_name().data(), nullptr, m_windowFlags))
    {
        render_window_implementation();
    }

    ImGui::End();
}