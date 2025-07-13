#include "collapsing_header.h"

#include "gluten/elements/button.h"
#include "gluten/elements/text.h"

auto gluten::collapsing_header::render_element(const ImRect& parentRect) -> bool
{
	gluten::background background;
    background.set_element_background_color(ImGui::GetStyleColorVec4(ImGuiCol_Header));
    background.set_element_hover_color(ImGui::GetStyleColorVec4(ImGuiCol_HeaderHovered));
    background.render(parentRect);

    gluten::element inner(element::anchor_preset::stretch_full);
    inner.set_element_padding(ImVec2(4.0f, 4.0f));
    inner.render(parentRect);

    const float text_offset_x = ImGui::GetCurrentContext()->FontSize + ImGui::GetStyle().FramePadding.x;

    gluten::text label(m_label, ImVec2(0.0f, 0.5f), anchor_preset::left_middle);
    label.set_element_translation(ImVec2(text_offset_x, 0.0f));
    label.render(inner.get_element_rect());

    ImGuiStorage* const storage = ImGui::GetStateStorage();
    if (!storage)
    {
        return m_defaultOpen;
    }

    bool open = storage->GetBool(ImGui::GetID(m_label.c_str()), m_defaultOpen);

    ImGui::RenderArrow(ImGui::GetWindowDrawList(), ImVec2(inner.get_element_rect().Min.x, inner.get_element_rect().Min.y + (inner.get_element_rect().GetHeight() / 2.0f) - (ImGui::GetWindowDrawList()->_Data->FontSize / 2.0f)), ImGui::GetColorU32(ImGuiCol_Text), open ? ImGuiDir_Down : ImGuiDir_Right);

    gluten::button button("##CollapsingHeaderButton", true, anchor_preset::stretch_full);
    if (button.render(parentRect))
    {
        open = !open;
        storage->SetBool(ImGui::GetID(m_label.c_str()), open);
    }

    return open;
}