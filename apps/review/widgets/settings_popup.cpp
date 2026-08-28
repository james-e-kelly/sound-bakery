#include "settings_popup.h"

#include "gluten/theme/things/things.h"

auto settings_popup::start_implementation() -> void
{
    if (m_userSettings->m_theme == review_app_theme::dark)
    {
        gluten::theme::things::apply_colours(gluten::theme::things::darkModeBackgroundColor, true);
    }
    else
    {
        gluten::theme::things::apply_colours(gluten::theme::things::lightModeBackgroundColor, false);
    }

    gluten::theme::apply_colours();
    get_app()->get_subsystem_by_class<gluten::widget_subsystem>()->refresh_style();
}

auto settings_popup::render_popup() -> void
{
    if (ImGui::BeginCombo("Theme", m_userSettings->m_theme == review_app_theme::dark ? "Dark" : "Light"))
    {
        bool darkSelected  = m_userSettings->m_theme == review_app_theme::dark;
        bool lightSelected = m_userSettings->m_theme == review_app_theme::light;

        if (ImGui::Selectable("Dark", &darkSelected))
        {
            m_userSettings->m_theme = review_app_theme::dark;
            gluten::theme::things::apply_colours(gluten::theme::things::darkModeBackgroundColor, true);
            gluten::theme::apply_colours();
            get_app()->get_subsystem_by_class<gluten::widget_subsystem>()->refresh_style();
        }

        if (ImGui::Selectable("Light", &lightSelected))
        {
            m_userSettings->m_theme = review_app_theme::light;
            gluten::theme::things::apply_colours(gluten::theme::things::lightModeBackgroundColor, false);
            gluten::theme::apply_colours();
            get_app()->get_subsystem_by_class<gluten::widget_subsystem>()->refresh_style();
        }

        ImGui::EndCombo();
    }
}