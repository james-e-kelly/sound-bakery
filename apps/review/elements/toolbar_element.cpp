#include "toolbar_element.h"

#include "managers/workspace_manager.h"
#include "widgets/settings_popup.h"

auto toolbar_element::refresh_element() -> void
{
    m_settingsPopup = gluten::app::get()->get_subsystem_by_class<gluten::widget_subsystem>()->add_widget_class_to_root<settings_popup>(false);
    m_settingsPopup->set_visibile(false);

    for (auto* button : {&m_toolbarReviewsButton, &m_toolbarUsersButton, &m_toolbarSettingsButton})
    {
        (*button)
            .set_icon_hover_grow(1.15f)
            .set_element_content_font_size(gluten::theme::iconSizeXl)
            .set_element_hover_color(gluten::theme::backgroundHover)
            .set_element_active_color(gluten::theme::backgroundActive)
            .set_element_padding(ImVec2(gluten::theme::space04, gluten::theme::space04))
            .set_element_rounding(gluten::theme::radiusMd)
            .set_element_anchor_preset(gluten::anchor_preset::stretch_full);
    }
}

auto toolbar_element::render_element(const gluten::element_render_info& renderInfo) -> bool
{
    if (std::shared_ptr<workspace_manager> workspaceManager = gluten::app::get()->get_manager_by_class<workspace_manager>())
    {
        bool toolbarHovered = get_element_is_hovered();

        m_toolbarReviewsButton.set_element_active(m_userSettings->m_activeView == review_app_view::reviews);
        m_toolbarUsersButton.set_element_active(m_userSettings->m_activeView == review_app_view::users);
        // m_toolbarSettingsButton.set_element_active(m_activeView == settings_view);

        render_layout_element_pixels_vertical(&m_toolbarButtonScaleBox, gluten::theme::space48);
        if (m_toolbarButtonScaleBox.render_child(&m_toolbarReviewsButton))
        {
            m_userSettings->m_activeView = review_app_view::reviews;
            workspaceManager->select_project({});
            workspaceManager->select_user({});
        }

        toolbarHovered &= !m_toolbarReviewsButton.get_element_is_hovered();

        render_spacer_pixels(0.0f, gluten::theme::space04);

        render_layout_element_pixels_vertical(&m_toolbarButtonScaleBox, gluten::theme::space48);
        if (m_toolbarButtonScaleBox.render_child(&m_toolbarUsersButton))
        {
            m_userSettings->m_activeView = review_app_view::users;
            workspaceManager->select_project({});
            workspaceManager->select_user({});
        }

        toolbarHovered &= !m_toolbarUsersButton.get_element_is_hovered();

        render_spacer_pixels(0.0f, get_remaining_layout_size().y - gluten::theme::space48);

        render_layout_element_pixels_vertical(&m_toolbarButtonScaleBox, gluten::theme::space48);
        if (m_toolbarButtonScaleBox.render_child(&m_toolbarSettingsButton))
        {
            m_settingsPopup->open_popup();
        }

        toolbarHovered &= !m_toolbarSettingsButton.get_element_is_hovered();

        if (toolbarHovered)
        {
            gluten::app::get()->get_subsystem_by_class<gluten::widget_subsystem>()->get_root_widget()->set_hovering_background(true);
        }
    }

    return false;
}