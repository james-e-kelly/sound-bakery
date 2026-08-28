#pragma once

#include "pch.h"

class create_project_popup;
class create_review_popup;
class user_settings_data;
class workspace_manager;

/**
 * @brief Renders a list of projects or reviews and opens them when clicked.
 */
class left_panel_element : public gluten::element
{
public:
    left_panel_element() : gluten::element(gluten::anchor_preset::stretch_full)
    {
    }

    auto set_up_panel(float headerHeight) -> void;

protected:
    auto refresh_element() -> void override;
    auto render_element(const gluten::element_render_info& renderInfo) -> bool override;

private:
    auto render_header(std::shared_ptr<workspace_manager>& manager) -> void;
    auto render_reviews_view(std::shared_ptr<workspace_manager>& manager) -> void;
    auto render_users_view(std::shared_ptr<workspace_manager>& manager) -> void;

    gluten::data_source<user_settings_data> m_userSettings;

    std::shared_ptr<create_project_popup> m_createProjectPopup;
    std::shared_ptr<create_review_popup> m_createReviewPopup;

    gluten::layout m_layout               = gluten::layout(gluten::layout_type::top_to_bottom, gluten::anchor_preset::stretch_full);
    gluten::layout m_itemsLayout          = gluten::layout(gluten::layout_type::top_to_bottom, gluten::anchor_preset::stretch_full);
    gluten::background m_headerBackground = gluten::background(gluten::anchor_preset::stretch_full);
    gluten::text m_titleText              = gluten::text({}, ImVec2(0.5f, 0.5f), gluten::element::anchor_preset::center_middle, gluten::text_style::h3);
    gluten::icon_button m_backButton      = gluten::icon_button("##BackButton", ICON_LC_CHEVRON_LEFT, gluten::fonts::regular_lucide_icons, gluten::button_style::secondary);
    gluten::icon_button m_newButton       = gluten::icon_button("##NewButton", ICON_LC_PLUS, gluten::fonts::regular_lucide_icons, gluten::button_style::secondary);

    float m_headerHeight = 30.0f;
    float m_itemHeight   = 15.0f;
};