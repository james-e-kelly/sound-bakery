#pragma once

#include "pch.h"

#include "gluten/elements/button.h"
#include "gluten/elements/text.h"

/**
 * @brief Generic element that goes in a list on a panel.
 * 
 * Project and review elements derive from this because they should both look the same.
 */
class list_element : public gluten::element
{
public:
    list_element() : gluten::element(anchor_preset::stretch_full) {}
    list_element(const std::string& title, const std::string& description, const std::string& detail)
        : gluten::element(anchor_preset::stretch_full),
          m_titleText(title, ImVec2(0.0f, 0.5f), gluten::anchor_preset::left_top, gluten::text_style::h3),
          m_descriptionText(description, ImVec2(0.0f, 0.0f), gluten::anchor_preset::stretch_middle, gluten::text_style::subtitle),
          m_detailText(detail, ImVec2(1.0f, 0.5f), gluten::anchor_preset::right_top, gluten::text_style::label, gluten::fonts::regular_lucide_icons)
    {
        set_element_inner_padding(gluten::theme::insetCompact);
        set_element_rounding(gluten::theme::radiusLg);
        set_element_hover_color(gluten::theme::fieldHover01);

        m_titleText.get_element_anchor().min = m_titleText.get_element_anchor().max = ImVec2(0.0f, 0.25f);
        m_descriptionText.get_element_anchor().max.y                                       = 1.0f;
        m_detailText.get_element_anchor().min = m_detailText.get_element_anchor().max = ImVec2(1.0f, 0.25f);
    }

protected:
    auto render_element(const gluten::element_render_info& renderInfo) -> bool override;

protected:
    gluten::text m_titleText;
    gluten::text m_descriptionText;
    gluten::text m_detailText;

    gluten::button m_projectButton = gluten::button("##ProjectElementButton", true, anchor_preset::stretch_full);
};