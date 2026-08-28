#include "button.h"

#include "gluten/theme/theme.h"
#include "gluten/utils/imgui_util_structures.h"
#include "imgui_internal.h"

gluten::button::button(const char* name, bool invisible, const anchor_preset& anchorPreset, const button_style& buttonStyle)
    : element(anchorPreset), m_name(name), m_invisible(invisible)
{
    set_button_style(buttonStyle);
}

auto gluten::button::set_button_style(button_style style) -> button&
{
    m_buttonStyle = style;
    return *this;
}

bool gluten::button::render_element(const element_render_info& renderInfo)
{
    std::optional<gluten::imgui::scoped_button_style> styleScope;
    if (m_buttonStyle.has_value())
    {
        styleScope.emplace(m_buttonStyle.value());
    }

    gluten::imgui::scoped_color_stack buttonColors(
        ImGuiCol_Button, m_activeColor.has_value() && m_active ? m_activeColor.value() : ImGui::GetColorU32(ImGuiCol_Button),
        ImGuiCol_ButtonActive, m_activeColor.has_value() && m_active ? m_activeColor.value() : ImGui::GetColorU32(ImGuiCol_Button),
        ImGuiCol_ButtonHovered, m_hoverColor.has_value() ? m_hoverColor.value() : ImGui::GetColorU32(ImGuiCol_ButtonHovered));

    const ImVec2 size = renderInfo.elementBox.GetSize();

    if (size.x < 0.1f || size.y < 0.1f)
    {
        return false;
    }

    if (m_invisible)
    {
        ImGui::SetNextItemAllowOverlap();
    }

    const bool activated = m_invisible ? ImGui::InvisibleButton(m_name, renderInfo.elementBox.GetSize())
                                       : ImGui::Button(m_name, renderInfo.elementBox.GetSize());

    return activated;
}

auto gluten::button::get_element_content_size(const ImVec2& parentSize) -> ImVec2 const
{
    const ImVec2 textSize = ImGui::CalcTextSize(m_name, nullptr, true);
    const ImVec2 padding  = ImGui::GetStyle().FramePadding;

    if (textSize.x > 0.0f)
    {
        return textSize + padding * 2;
    }

    return m_minSize;
}