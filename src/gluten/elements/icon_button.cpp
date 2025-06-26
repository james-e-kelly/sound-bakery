#include "icon_button.h"

gluten::icon_button::icon_button(const char* buttonID, const char* icon, fonts font)
    : m_button(buttonID, true), m_text(icon)
{
    m_button.get_element_anchor().set_achor_from_preset(anchor_preset::stretch_full);
    m_text.get_element_anchor().set_achor_from_preset(anchor_preset::center_middle);

    m_text.set_font(font);

    m_text.set_element_alignment(ImVec2(0.25f, 0.25f));
}

bool gluten::icon_button::render_element(const ImRect& parent)
{
    ImGui::BeginGroup();
    const bool buttonActivated = m_button.render_element(parent);
    m_text.set_element_scale(get_element_scale());
    m_text.render(parent);
    ImGui::EndGroup();
    return buttonActivated;
}

auto gluten::icon_button::set_element_min_size(const ImVec2& minSize) -> element&
{
    element::set_element_min_size(minSize);
    m_button.set_element_min_size(minSize);
    m_text.set_element_min_size(minSize);
    return *this;
}

auto gluten::icon_button::set_element_max_size(const ImVec2& maxSize) -> element&
{
    element::set_element_max_size(maxSize);
    m_button.set_element_max_size(maxSize);
    m_text.set_element_max_size(maxSize);
    return *this;
}