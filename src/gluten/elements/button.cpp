#include "button.h"

#include "gluten/theme/theme.h"
#include "imgui_internal.h"

gluten::button::button(const char* name, bool invisible, const anchor_preset& anchorPreset)
    : element(anchorPreset), m_name(name), m_invisible(invisible)
{
}

bool gluten::button::render_element(const ImRect& elementRect)
{
    const ImVec2 size = elementRect.GetSize();

    if (size.x < 0.1f || size.y < 0.1f)
    {
        return false;
    }

    if (m_invisible)
    {
        ImGui::SetNextItemAllowOverlap();
    }

    const bool activated = m_invisible ? ImGui::InvisibleButton(m_name, elementRect.GetSize())
                                       : ImGui::Button(m_name, elementRect.GetSize());

    return activated;
}

auto gluten::button::get_element_content_size() -> ImVec2 const
{
    const ImVec2 textSize = ImGui::CalcTextSize(m_name);
    const ImVec2 padding  = ImGui::GetStyle().FramePadding;
    return ImVec2(textSize.x + padding.x * 2, textSize.y + padding.y * 2);
}