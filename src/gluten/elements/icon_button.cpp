#include "icon_button.h"

#include "gluten/app/app.h"

gluten::icon_button::icon_button(const char* buttonID, const char* icon, fonts font)
    : m_button(buttonID, true), m_text(icon)
{
    set_element_anchor_preset(anchor_preset::stretch_full);
    
    m_button.set_element_anchor_preset(anchor_preset::stretch_full);

    m_text
        .set_font(font)
        .set_element_anchor_preset(anchor_preset::center_middle)
        .set_element_alignment(ImVec2(0.5f, 0.5f));
}

bool gluten::icon_button::render_element(const ImRect& parent)
{
    ImGui::BeginGroup();
    m_button.set_element_min_size(m_text.get_element_content_size(parent.GetSize()));
    const bool buttonActivated = m_button.render(parent);
    m_text.set_element_content_scale(get_element_scale());
    m_text.render(parent);
    ImGui::EndGroup();
    return buttonActivated;
}

auto gluten::icon::get_element_content_size(const ImVec2& parentSize) -> ImVec2 const
{
    const float scale = get_element_scale();
    return ImVec2(g_baseIconFontSize * scale, g_baseIconFontSize * scale); 
}

auto gluten::icon::pre_render_element() -> void
{
    if (m_font.has_value())
    {
        ImGui::PushFont(gluten::app::get()->get_font(m_font.value()));
    }
}