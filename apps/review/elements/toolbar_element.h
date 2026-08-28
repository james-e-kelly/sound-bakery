#pragma once

#include "pch.h"

#include "gluten/elements/button.h"
#include "gluten/elements/text.h"

class user_settings_data;
class settings_popup;

/**
 * @brief Renders a vertical toolbar of main buttons.
 */
class toolbar_element : public gluten::layout
{
public:
    toolbar_element() : gluten::layout(layout_type::top_to_bottom, anchor_preset::stretch_full) 
    {
        
    }

    auto refresh_element() -> void override;

protected:
    auto render_element(const gluten::element_render_info& renderInfo) -> bool override;

private:
    gluten::scale_box m_toolbarButtonScaleBox   = gluten::scale_box(gluten::anchor_preset::stretch_full);
    gluten::icon_button m_toolbarReviewsButton  = gluten::icon_button("##ReviewsButton", ICON_LC_FOLDER_KANBAN, gluten::fonts::regular_lucide_icons);
    gluten::icon_button m_toolbarUsersButton    = gluten::icon_button("##UsersButton", ICON_LC_USERS, gluten::fonts::regular_lucide_icons);
    gluten::icon_button m_toolbarSettingsButton = gluten::icon_button("##SettingsButton", ICON_LC_SETTINGS, gluten::fonts::regular_lucide_icons);

    std::shared_ptr<settings_popup> m_settingsPopup;

    gluten::data_source<user_settings_data> m_userSettings;
};