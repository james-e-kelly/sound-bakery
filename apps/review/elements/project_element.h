#include "pch.h"

#include "gluten/elements/element.h"

class project_element : public gluten::element
{
public:
    project_element() : gluten::element(anchor_preset::stretch_full) {}
    project_element(const std::string& projectName,
                    const std::string& projectDescription,
                    int openReviews,
                    int closedReviews,
                    int archivedReviews)
        : gluten::element(anchor_preset::stretch_full), m_projectName(projectName), m_projectDescription(projectDescription),
          m_openReviews(openReviews),
          m_closedReviews(closedReviews),
          m_archivedReviews(archivedReviews)
    {
        set_element_background_color(gluten::theme::carbon_g100::field03);
    }

protected:
    auto render_element(const ImRect& elementRect) -> bool override
    {
        gluten::imgui::scoped_id id(ImGui::GetID(m_projectName.c_str()));

        if (m_backgroundColor.has_value())
        {
            gluten::background background;
            background
                .set_element_background_color(m_backgroundColor.value())
                .set_element_hover_color(gluten::theme::carbon_g100::fieldHover03);
            m_backgroundColor.reset();
            background.render(elementRect);
        }

        ImRect contentRect = elementRect;
        contentRect.Expand(ImVec2(-5.0f, -5.0f));

        gluten::text projectTitleText(m_projectName.c_str(), ImVec2(0.0f, 0.5f), anchor_preset::left_top);
        projectTitleText
            .set_font(gluten::fonts::title)
            .set_element_content_font_size(20.0f)
            .get_element_anchor().min = projectTitleText.get_element_anchor().max = ImVec2(0.0f, 0.25f);

        gluten::text projectDescriptionText(m_projectDescription.c_str(), ImVec2(0.0f, 0.0f), anchor_preset::left_top);
        projectDescriptionText.set_element_content_font_size(16.0f).get_element_anchor().min = projectTitleText.get_element_anchor().max = ImVec2(0.0f, 0.5f);

        const std::string reviewText = fmt::format("{} {} | {} {} | {} {}", m_openReviews, ICON_LC_PENCIL, m_closedReviews, ICON_LC_CHECK_LINE, m_archivedReviews, ICON_LC_ARCHIVE);

        gluten::text openReviewsText(reviewText.c_str(), ImVec2(1.0f, 0.5f), anchor_preset::right_top);
        openReviewsText
            .set_font(gluten::fonts::regular_lucide_icons)
            .set_element_content_font_size(20.0f)
            .get_element_anchor().min = openReviewsText.get_element_anchor().max = ImVec2(1.0f, 0.25f);

        gluten::button button("##ProjectElementButton", true, anchor_preset::stretch_full);

        projectTitleText.render(contentRect);
        projectDescriptionText.render(contentRect);
        openReviewsText.render(contentRect);

        return button.render(elementRect);
    }

private:
    std::string m_projectName;
    std::string m_projectDescription;
    int m_openReviews = 0;
    int m_closedReviews = 0;
    int m_archivedReviews = 0;
};