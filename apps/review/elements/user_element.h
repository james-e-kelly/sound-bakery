#include "pch.h"

#include "data/user_data.h"
#include "gluten/elements/element.h"

class user_element : public gluten::element
{
public:
    user_element() = delete;
    user_element(const user_data& user)
        : gluten::element(anchor_preset::stretch_full),
          m_user(user)
    {
        set_element_background_color(gluten::theme::field03);
    }

protected:
    auto render_element(const gluten::element_render_info& renderInfo) -> bool override
    {
        gluten::imgui::scoped_id id(ImGui::GetID(m_user.m_email.c_str()));

        if (m_backgroundColor.has_value())
        {
            gluten::background background;
            background.set_element_background_color(m_backgroundColor.value())
                .set_element_hover_color(gluten::theme::fieldHover03);
            m_backgroundColor.reset();
            background.render(renderInfo.elementBox);
        }

        gluten::element inner(gluten::element::anchor_preset::stretch_full);
        inner.set_element_padding(ImVec2(5.0f, 5.0f));
        inner.render(renderInfo.elementBox);

        gluten::text reviewTitleText(m_user.m_displayName, ImVec2(0.0f, -0.25f), anchor_preset::left_top);
        reviewTitleText.set_font(gluten::fonts::title).set_element_content_font_size(18.0f);
        reviewTitleText.get_element_anchor().min = ImVec2(0.0f, 0.0f);
        reviewTitleText.get_element_anchor().max = ImVec2(0.8f, 0.5f);

        gluten::text reviewDescriptionText(m_user.m_title, ImVec2(0.0f, 0.0f), anchor_preset::left_top);
        reviewDescriptionText.set_element_content_font_size(16.0f);
        reviewDescriptionText.get_element_anchor().min = ImVec2(0.0f, 0.5f);
        reviewDescriptionText.get_element_anchor().max = ImVec2(0.8f, 1.0f);

        const std::string reviewText = fmt::format("{}", m_user.m_privileges == user_privileges::admin ? ICON_LC_USER_COG : m_user.m_privileges == user_privileges::user ? ICON_LC_USER_PEN
                                                                                                                                                                         : ICON_LC_USER);

        gluten::text openReviewsText(reviewText.c_str(), ImVec2(1.0f, -1.0f), anchor_preset::right_top);
        openReviewsText
            .set_font(gluten::fonts::regular_lucide_icons)
            .set_element_content_font_size(20.0f);

        gluten::button button("##UserElementButton", true, anchor_preset::stretch_full);

        reviewTitleText.render(inner.get_element_rect());
        reviewDescriptionText.render(inner.get_element_rect());
        openReviewsText.render(inner.get_element_rect());

        return button.render(renderInfo.elementBox);
    }

private:
    const user_data& m_user;
};