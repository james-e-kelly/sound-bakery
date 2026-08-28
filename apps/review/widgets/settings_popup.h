#include "pch.h"

#include "data/user_settings_data.h"

class settings_popup : public gluten::popup_widget
{
    WIDGET_CONSTRUCT_PARENT(settings_popup, "Settings", gluten::popup_widget)

protected:
    auto start_implementation() -> void override;
    auto render_popup() -> void override;

private:
    gluten::data_source<user_settings_data> m_userSettings;
};