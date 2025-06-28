#include "icon_button.h"

#include "gluten/app/app.h"

gluten::icon_button::icon_button(const char* buttonID, const char* icon, fonts font)
    : m_button(buttonID, true), m_text(icon)
{
    set_element_anchor_preset(anchor_preset::stretch_full);
    m_button.get_element_anchor().set_achor_from_preset(anchor_preset::stretch_full);

    m_text
        .set_font(font)
        .set_element_anchor_preset(anchor_preset::center_middle)
        .set_element_alignment(ImVec2(0.5f, 0.5f));
}

bool gluten::icon_button::render_element(const ImRect& parent)
{
    ImGui::BeginGroup();
    m_button.set_element_min_size(m_text.get_element_content_size());
    m_button.set_element_content_scale(get_element_scale());
    const bool buttonActivated = m_button.render(parent);
    m_text.set_element_content_scale(get_element_scale());
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

auto gluten::icon_button::set_element_content_scale(float scale) -> element&
{
    element::set_element_content_scale(scale);
    m_button.set_element_content_scale(scale);
    m_text.set_element_content_scale(scale);
    return *this;
}

auto gluten::icon_button::set_icon_alignment(const ImVec2& alignment) -> icon_button&
{
    m_text.set_element_alignment(alignment);
    return *this;
}

auto gluten::icon_button::set_button_alignment(const ImVec2& alignment) -> icon_button&
{
    m_button.set_element_alignment(alignment);
    return *this;
}

auto gluten::icon_button::set_element_active(bool active) -> element&
{
    m_button.set_element_active(active);
    return *this;
}

auto gluten::icon_button::set_button_scale(float scale) -> icon_button&
{
    m_button.set_element_scale(scale);
    return *this;
}

auto gluten::icon_button::set_element_background_color(ImU32 color) -> element& 
{
    m_button.set_element_background_color(color);
    return *this;
}

auto gluten::icon_button::set_element_background_color(ImVec4 color) -> element&
{
    m_button.set_element_background_color(color);
    return *this;
}

auto gluten::icon_button::set_element_hover_color(ImU32 color) -> element& 
{
    m_button.set_element_hover_color(color);
    return *this;
}

auto gluten::icon_button::set_element_hover_color(ImVec4 color) -> element& 
{
    m_button.set_element_hover_color(color);
    return *this;
}

auto gluten::icon_button::set_element_active_color(ImU32 color) -> element& 
{
    m_button.set_element_active_color(color);
    return *this;
}

auto gluten::icon_button::set_element_active_color(ImVec4 color) -> element& 
{
    m_button.set_element_active_color(color);
    return *this;
}

auto gluten::icon::get_element_content_size() -> ImVec2 const
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
