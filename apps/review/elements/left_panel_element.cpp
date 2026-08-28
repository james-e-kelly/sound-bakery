#include "left_panel_element.h"

#include "data/user_settings_data.h"
#include "elements/project_element.h"
#include "elements/review_element.h"
#include "elements/user_element.h"
#include "managers/workspace_manager.h"
#include "widgets/create_project_popup.h"
#include "widgets/create_review_popup.h"

auto left_panel_element::set_up_panel(float headerHeight) -> void
{
    m_headerHeight = headerHeight;
    m_itemHeight   = gluten::theme::space48;
    refresh_element();
}

auto left_panel_element::refresh_element() -> void
{
    m_createProjectPopup = gluten::app::get()->get_subsystem_by_class<gluten::widget_subsystem>()->add_widget_class_to_root<create_project_popup>(false);
    m_createReviewPopup  = gluten::app::get()->get_subsystem_by_class<gluten::widget_subsystem>()->add_widget_class_to_root<create_review_popup>(false);
    
    m_layout
        .set_layout_padding(gluten::theme::insetFrame)
        .set_element_background_color(gluten::theme::layer01)
        .set_element_rounding(gluten::theme::radiusLg)
        .set_element_rounding_flags(ImDrawFlags_RoundCornersLeft);

    m_itemsLayout
        .set_layout_spacing(gluten::theme::space04);

    m_headerBackground
        .set_element_border_sides(gluten::border_sides::bottom)
        .set_element_border_color(gluten::theme::borderStrong01)
        .set_element_inner_padding(gluten::theme::insetCompact);

    m_titleText
        .set_text_style(gluten::text_style::h3);

    for (auto* button : {&m_backButton, &m_newButton})
    {
        (*button)
            .set_icon_hover_grow(1.15f)
            .set_button_style(gluten::button_style::secondary)
            .set_element_hover_color(gluten::theme::fieldHover01)
            .set_element_active_color(gluten::theme::layerActive01)
            .set_element_content_font_size(gluten::theme::iconSizeLg)
            .set_element_rounding(gluten::theme::radiusMd);
    }

    m_backButton
        .set_element_anchor_preset(gluten::anchor_preset::left_middle);

    m_newButton
        .set_element_anchor_preset(gluten::anchor_preset::right_middle);
}

auto left_panel_element::render_header(std::shared_ptr<workspace_manager>& manager) -> void
{
    m_layout.render_layout_element_pixels_vertical(&m_headerBackground, m_headerHeight);

    switch (m_userSettings->m_activeView)
    {
        case review_app_view::reviews:
        {
            if (!manager->has_selected_project())
            {
                m_titleText.set_text("Projects");
                m_titleText.set_element_anchor_preset(gluten::anchor_preset::left_middle);
                m_titleText.render(m_headerBackground.get_element_content_rect());

                if (m_userSettings->m_loggedInUser.m_privileges > user_privileges::guest)
                {
                    if (m_newButton.render(m_headerBackground.get_element_content_rect()))
                    {
                        m_createProjectPopup->open_popup();
                    }
                }
            }
            else
            {
                m_titleText.set_text("Reviews");
                m_titleText.set_element_anchor_preset(gluten::anchor_preset::center_middle);
                m_titleText.render(m_headerBackground.get_element_content_rect());

                if (m_backButton.render(m_headerBackground.get_element_content_rect()))
                {
                    manager->select_project({});
                }

                if (m_userSettings->m_loggedInUser.m_privileges > user_privileges::guest)
                {
                    if (m_newButton.render(m_headerBackground.get_element_content_rect()))
                    {
                        m_createReviewPopup->open_popup();
                    }
                }

            }
            break;
        }
        case review_app_view::users:
        {
            m_titleText.set_text("Users");
            m_titleText.render(m_headerBackground.get_element_content_rect());

            if (m_userSettings->m_loggedInUser.m_privileges > user_privileges::guest)
            {
                if (m_newButton.render(m_headerBackground.get_element_content_rect()))
                {
                    manager->open_create_user_popup();
                }
            }
            break;
        }
        default:
            break;
    }
}

auto left_panel_element::render_reviews_view(std::shared_ptr<workspace_manager>& manager) -> void
{
    if (!manager->has_selected_project())
    {
        const auto& allProjects = manager->get_all_projects();

        if (allProjects.has_data())
        {
            for (const auto& project : allProjects.m_cache)
            {
                project_element projectElement(project.m_projectName, project.m_projectDescription, project.m_openReviews, project.m_closedReviews, project.m_archivedReviews);
                if (m_itemsLayout.render_layout_element_pixels_vertical(&projectElement, m_itemHeight))
                {
                    manager->select_project(project.m_projectName);
                    //m_editReviewers.set_project_id(project.m_id);
                }
            }
        }
        else
        {
            gluten::loading_spinner loadingSpinner;
            m_itemsLayout.render_layout_element_pixels_vertical(&loadingSpinner, m_itemHeight);
        }
    }
    else
    {
        const auto& allReviews = manager->get_all_reviews();

        auto render_reviews = [&allReviews, &manager, this](review_status statusToRender)
        {
            if (allReviews.has_data())
            {
                for (const auto& review : allReviews.m_cache)
                {
                    if (review.m_reviewStatus == statusToRender)
                    {
                        review_element reviewElement(review);
                        if (m_itemsLayout.render_layout_element_pixels_vertical(&reviewElement, m_itemHeight))
                        {
                            manager->select_review(review.m_reviewId);
                        }
                    }
                }
                m_itemsLayout.render_vertical_spacer(gluten::theme::space08);
            }
            else
            {
                gluten::loading_spinner loadingSpinner;
                m_itemsLayout.render_layout_element_pixels_vertical(&loadingSpinner, m_itemHeight);
            }
        };

        if (allReviews.has_data())
        {
            const int openSum = std::count_if(allReviews.m_cache.begin(), allReviews.m_cache.end(), [](const auto& review) { return review.m_reviewStatus == review_status::open; });
            const int closedSum = std::count_if(allReviews.m_cache.begin(), allReviews.m_cache.end(), [](const auto& review) { return review.m_reviewStatus == review_status::closed; });
            const int archivedSum = std::count_if(allReviews.m_cache.begin(), allReviews.m_cache.end(), [](const auto& review) { return review.m_reviewStatus == review_status::archived; });

            gluten::collapsing_header openReviewsHeader("Open", true, openSum ? std::to_string(openSum) : "");
            gluten::collapsing_header closedReviewsHeader("Closed", false, closedSum ? std::to_string(closedSum) : "");
            gluten::collapsing_header archivedReviewsHeader("Archived", false, archivedSum ? std::to_string(archivedSum) : "");

            if (m_itemsLayout.render_layout_element_pixels_vertical(&openReviewsHeader, m_itemHeight / 2.0f))
            {
                m_itemsLayout.render_spacer_pixels(0.0f, gluten::theme::space04);
                render_reviews(review_status::open);
            }

            if (m_itemsLayout.render_layout_element_pixels_vertical(&closedReviewsHeader, m_itemHeight / 2.0f))
            {
                m_itemsLayout.render_spacer_pixels(0.0f, gluten::theme::space04);
                render_reviews(review_status::closed);
            }

            if (m_itemsLayout.render_layout_element_pixels_vertical(&archivedReviewsHeader, m_itemHeight / 2.0f))
            {
                m_itemsLayout.render_spacer_pixels(0.0f, gluten::theme::space04);
                render_reviews(review_status::archived);
            }
        }
    }
}

auto left_panel_element::render_users_view(std::shared_ptr<workspace_manager>& manager) -> void
{
    const auto& allUsers = manager->get_all_users();

    if (allUsers.has_data())
    {
        for (const auto& user : allUsers.m_cache)
        {
            user_element userElement(user);
            if (m_itemsLayout.render_layout_element_pixels_vertical(&userElement, m_itemHeight))
            {
                manager->select_user(user.m_email);
            }
        }
    }
    else
    {
        gluten::loading_spinner loadingSpinner;
        m_itemsLayout.render_layout_element_pixels_vertical(&loadingSpinner, m_itemHeight);
    }
}

auto left_panel_element::render_element(const gluten::element_render_info& renderInfo) -> bool
{
    if (std::shared_ptr<workspace_manager> workspaceManager = gluten::app::get()->get_manager_by_class<workspace_manager>())
    {
        m_layout.render(renderInfo.elementBox);

        render_header(workspaceManager);
    
        m_layout.render_layout_element_remaining(&m_itemsLayout);
        m_itemsLayout.render_vertical_spacer(gluten::theme::space04);
    
        switch (m_userSettings->m_activeView)
        {
            case review_app_view::reviews:
                render_reviews_view(workspaceManager);            
                break;
            case review_app_view::users:
                render_users_view(workspaceManager);
                break;
            default:
                break;
        }
    }

    return false;
}