#include "pch.h"

#include "gluten/elements/element.h"

class review_element : public gluten::element
{
public:
    review_element() = delete;
    review_element(const review_data& review)
        : gluten::element(anchor_preset::stretch_full),
        m_review(review)
    {
        set_element_background_color(gluten::theme::carbon_g100::field03);
    }

protected:
    auto render_element(const ImRect& elementRect) -> bool override
    {
        gluten::imgui::scoped_id id(ImGui::GetID(m_review.m_reviewId));

        if (m_backgroundColor.has_value())
        {
            gluten::background background;
            background.set_element_background_color(m_backgroundColor.value())
                .set_element_hover_color(gluten::theme::carbon_g100::fieldHover03);
            m_backgroundColor.reset();
            background.render(elementRect);
        }

        ImRect contentRect = elementRect;
        contentRect.Expand(ImVec2(-5.0f, -5.0f));

        gluten::text reviewTitleText(m_review.m_reviewName.c_str(), ImVec2(0.0f, 0.5f), anchor_preset::left_top);
        reviewTitleText.set_font(gluten::fonts::title).set_element_content_font_size(20.0f).get_element_anchor().min =
            reviewTitleText.get_element_anchor().max = ImVec2(0.0f, 0.25f);

        gluten::text reviewDescriptionText(m_review.m_reviewDescription.c_str(), ImVec2(0.0f, 0.0f), anchor_preset::left_top);
        reviewDescriptionText.set_element_content_font_size(16.0f).get_element_anchor().min =
            reviewDescriptionText.get_element_anchor().max = ImVec2(0.0f, 0.5f);

        const std::string reviewText =
            fmt::format("{}", m_review.m_reviewStatus == review_status::open ? ICON_LC_EYE : ICON_LC_CHECK_LINE);

        gluten::text openReviewsText(reviewText.c_str(), ImVec2(1.0f, 0.5f), anchor_preset::right_top);
        openReviewsText.set_font(gluten::fonts::regular_lucide_icons)
            .set_element_content_font_size(20.0f)
            .get_element_anchor()
            .min = openReviewsText.get_element_anchor().max = ImVec2(1.0f, 0.25f);

        gluten::button button("##ProjectElementButton", true, anchor_preset::stretch_full);

        reviewTitleText.render(contentRect);
        reviewDescriptionText.render(contentRect);
        openReviewsText.render(contentRect);

        return button.render(elementRect);
    }

private:
    const review_data& m_review;
};