#include "project_element.h"

#include "managers/workspace_manager.h"

auto project_element::render_element(const ImRect& elementRect) -> bool
{
    gluten::imgui::scoped_id id(ImGui::GetID(m_projectName.c_str()));

    if (m_backgroundColor.has_value())
    {
        gluten::background background;
        background
            .set_element_background_color(m_backgroundColor.value())
            .set_element_hover_color(gluten::theme::carbon_g100::fieldHover03)
            .set_element_rounding(m_elementRounding);
        m_backgroundColor.reset();
        background.render(elementRect);
    }

    ImRect contentRect = elementRect;
    contentRect.Expand(ImVec2(-5.0f, -5.0f));

    gluten::text projectTitleText(m_projectName.c_str(), ImVec2(0.0f, 0.5f),
                                  anchor_preset::left_top);
    projectTitleText.set_font(gluten::fonts::title)
        .set_element_content_font_size(20.0f)
        .get_element_anchor()
        .min = projectTitleText.get_element_anchor().max = ImVec2(0.0f, 0.25f);

    gluten::text projectDescriptionText(m_projectDescription.c_str(),
                                        ImVec2(0.0f, 0.0f),
                                        anchor_preset::left_top);
    projectDescriptionText.set_element_content_font_size(16.0f)
        .get_element_anchor()
        .min = projectTitleText.get_element_anchor().max = ImVec2(0.0f, 0.5f);

    const std::string reviewText = fmt::format(
        "{} {} | {} {} | {} {}", m_openReviews, ICON_LC_PENCIL, m_closedReviews,
        ICON_LC_CHECK_LINE, m_archivedReviews, ICON_LC_ARCHIVE);

    gluten::text openReviewsText(reviewText.c_str(), ImVec2(1.0f, 0.5f),
                                 anchor_preset::right_top);
    openReviewsText.set_font(gluten::fonts::regular_lucide_icons)
        .set_element_content_font_size(20.0f)
        .get_element_anchor()
        .min = openReviewsText.get_element_anchor().max = ImVec2(1.0f, 0.25f);

    gluten::button button("##ProjectElementButton", true,
                          anchor_preset::stretch_full);

    projectTitleText.render(contentRect);
    {
        gluten::imgui::scoped_color textCol(ImGuiCol_Text, gluten::theme::carbon_g100::textSecondary);
        projectDescriptionText.render(contentRect);
    }
    openReviewsText.render(contentRect);

    const bool pressed = button.render(elementRect);

    if (ImGui::BeginPopupContextItem("Project Context"))
    {
        if (std::shared_ptr<workspace_manager> workspaceManager = gluten::app::get()->get_manager_by_class<workspace_manager>())
        {
            if (workspaceManager->get_user_privileges() == user_privileges::admin)
            {
                if (ImGui::Selectable("Delete Project"))
                {
                    workspaceManager->delete_project(m_projectName);
                }
            }
        }

        ImGui::EndPopup();
    }

    return pressed;
}
