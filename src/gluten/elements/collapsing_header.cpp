#include "collapsing_header.h"

#include "gluten/elements/button.h"
#include "gluten/elements/text.h"
#include "gluten/utils/imgui_util_structures.h"

#include "IconsLucide.h"

auto gluten::collapsing_header::set_open(bool open) -> void
{
    gluten::imgui::scoped_id id(m_label.c_str());

    if (ImGuiStorage* const storage = ImGui::GetStateStorage())
    {
        storage->SetBool(ImGui::GetID(m_label.c_str()), open);
    }
}

auto gluten::collapsing_header::get_open() const -> bool
{
    gluten::imgui::scoped_id id(m_label.c_str());

    bool result = false;

    if (ImGuiStorage* const storage = ImGui::GetStateStorage())
    {
        result = storage->GetBool(ImGui::GetID(m_label.c_str()));
    }

    return false;
}

auto gluten::collapsing_header::render_element(const element_render_info& renderInfo) -> bool
{
    gluten::imgui::scoped_id id(m_label.c_str());

    gluten::text label(m_label, ImVec2(0.0f, 0.5f), anchor_preset::left_middle);
    label.set_element_translation(ImVec2(gluten::theme::space24, 0.0f));
    label.render(renderInfo.elementBox);

    ImGuiStorage* const storage = ImGui::GetStateStorage();
    if (!storage)
    {
        return m_defaultOpen;
    }

    bool open = storage->GetBool(ImGui::GetID(m_label.c_str()), m_defaultOpen);

    gluten::text iconText(open ? ICON_LC_CHEVRON_DOWN : ICON_LC_CHEVRON_RIGHT, ImVec2(0.0, 0.5f), anchor_preset::left_middle);
    iconText.set_text_style(text_style::subtitle, fonts::regular_lucide_icons);

    iconText.render(renderInfo.elementBox);

    if (!m_detail.empty())
    {
        gluten::text detailText(m_detail, ImVec2(1.0f, 0.5f), anchor_preset::right_middle, text_style::subtitle);
        detailText.render(renderInfo.elementBox);
    }

    gluten::button button("##CollapsingHeaderButton", true, anchor_preset::stretch_full);
    if (button.render(renderInfo.elementBox))
    {
        open = !open;
        storage->SetBool(ImGui::GetID(m_label.c_str()), open);
    }

    return open;
}