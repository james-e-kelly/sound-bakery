#include "review_element.h"

#include "managers/workspace_manager.h"

auto review_element::render_element(const ImRect& elementRect) -> bool
{
    gluten::imgui::scoped_id id(ImGui::GetID(m_review.m_reviewId));

    ImGui::BeginGroup();

    if (m_backgroundColor.has_value())
    {
        gluten::background background;
        background.set_element_background_color(m_backgroundColor.value())
            .set_element_hover_color(gluten::theme::carbon_g100::fieldHover03);
        m_backgroundColor.reset();
        background.render(elementRect);
    }

    gluten::element inner(gluten::element::anchor_preset::stretch_full);
    inner.set_element_padding(ImVec2(5.0f, 5.0f));
    inner.render(elementRect);

    gluten::text reviewTitleText(m_review.m_reviewName.c_str(), ImVec2(0.0f, -0.25f), anchor_preset::left_top);
    reviewTitleText.set_font(gluten::fonts::title).set_element_content_font_size(18.0f);
    reviewTitleText.get_element_anchor().min = ImVec2(0.0f, 0.0f);
    reviewTitleText.get_element_anchor().max = ImVec2(0.8f, 0.5f);

    gluten::text reviewDescriptionText(m_review.m_reviewDescription.c_str(), ImVec2(0.0f, 0.0f),
                                       anchor_preset::left_top);
    reviewDescriptionText.set_element_content_font_size(16.0f);
    reviewDescriptionText.get_element_anchor().min = ImVec2(0.0f, 0.5f);
    reviewDescriptionText.get_element_anchor().max = ImVec2(0.8f, 1.0f);

    const std::string reviewText =
        fmt::format("{}", m_review.m_reviewStatus == review_status::open ? ICON_LC_EYE : ICON_LC_CHECK_LINE);

    gluten::text openReviewsText(reviewText.c_str(), ImVec2(1.0f, -1.0f), anchor_preset::right_top);
    openReviewsText.set_font(gluten::fonts::regular_lucide_icons).set_element_content_font_size(20.0f);

    gluten::button button("##ProjectElementButton", true, anchor_preset::stretch_full);

    reviewTitleText.render(inner.get_element_rect());
    reviewDescriptionText.render(inner.get_element_rect());
    openReviewsText.render(inner.get_element_rect());

    const bool pressed = button.render(elementRect);

    ImGui::EndGroup();

    if (ImGui::BeginPopupContextItem("Review Context"))
    {
        if (ImGui::Selectable("Delete Review"))
        {
            if (std::shared_ptr<workspace_manager> workspaceManager =
                gluten::app::get()->get_manager_by_class<workspace_manager>())
            {
                workspaceManager->delete_review(m_review.m_reviewId);
            }
        }

        ImGui::EndPopup();
    }

    return pressed;
}