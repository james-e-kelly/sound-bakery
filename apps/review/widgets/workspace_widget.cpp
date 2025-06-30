#include "workspace_widget.h"

#include "IconsLucide.h"
#include "gluten/elements/layouts/layout.h"
#include "gluten/elements/icon_button.h"
#include "gluten/theme/carbon_theme_g100.h"
#include "gluten/utils/imgui_util_structures.h"

#include "managers/workspace_manager.h"

namespace
{
    constexpr float leftToobarWidth             = 100.0f;
    constexpr float leftToolbarButtonHeight     = leftToobarWidth;
    constexpr float leftToolbarHalfButtonHeight = leftToolbarButtonHeight / 2.0f;

    constexpr float itemListWidth = leftToobarWidth * 5.0f;

    constexpr auto get_review_phase_name(review_phase phase) -> const char*
    {
        const char* name = nullptr;

        switch (phase)
        {
            case review_phase::temp:
                name = "Temp";
                break;
            case review_phase::first_pass:
                name = "First Pass";
                break;
            case review_phase::iteration_pass:
                name = "Iteration Pass";
                break;
            case review_phase::final_pass:
                name = "Final Pass Pass";
                break;
            default:
                break;
        }

        return name;
    }

    constexpr auto get_review_quality_name(review_quality quality) -> const char*
    {
        const char* name = nullptr;

        switch (quality)
        {
            case review_quality::unknown:
                name = "Unknown";
                break;
            case review_quality::c:
                name = "C";
                break;
            case review_quality::b:
                name = "B";
                break;
            case review_quality::a:
                name = "A";
                break;
            case review_quality::a_plus:
                name = "A+";
                break;
            case review_quality::a_plus_plus:
                name = "A++";
                break;
            default:
                break;
        }

        return name;
    }
}

class project_element : public gluten::element
{
public:
    project_element() : gluten::element(anchor_preset::stretch_full) {}
    project_element(const std::string& projectName,
                    const std::string& projectDescription,
                    int openReviews,
                    int closedReviews)
        : gluten::element(anchor_preset::stretch_full), m_projectName(projectName), m_projectDescription(projectDescription),
          m_openReviews(openReviews),
          m_closedReviews(closedReviews)
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

        const std::string reviewText = fmt::format("{} {} {} {}", m_openReviews, ICON_LC_PENCIL, m_closedReviews, ICON_LC_CHECK);

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
};

auto create_project_popup::render_popup() -> void
{
    ImGui::SetWindowFontScale(1.5f);

    ImGui::InputTextWithHint("Project Name", "My New Project", projectNameBuffer, textBufferSize);

    ImGui::InputTextWithHint("Project Description", "2D platformer metroidvania", projectDescriptionBuffer, textBufferSize);

    const std::string projectName        = projectNameBuffer;
    const std::string projectDescription = projectDescriptionBuffer;

    const bool setupValid = !projectName.empty();

    ImGui::BeginDisabled(!setupValid);

    if (ImGui::Button("Create"))
    {
        if (std::shared_ptr<workspace_manager> workspaceManager = get_app()->get_manager_by_class<workspace_manager>())
        {
            workspaceManager->create_project(projectName, projectDescription);
        }

        close_popup();
    }

    ImGui::EndDisabled();

    ImGui::SameLine();

    if (ImGui::Button("Cancel"))
    {
        close_popup();
    }

    ImGui::SetWindowFontScale(1.0f);
}

auto create_review_popup::render_popup() -> void
{
    ImGui::SetWindowFontScale(1.5f);

    ImGui::InputTextWithHint("Review Name", "My New Review", reviewNameBuffer, textBufferSize);
    ImGui::SetItemTooltip("Write the title of the review. It can be anything from \"Adding some sounds\" to "
                          "\"[TASK-1234][Level1] Add Looping Fire Sounds\"");

    ImGui::InputTextWithHint("Review Description", "Adding a new sound", reviewDescriptionBuffer,
                             textBufferSize);
    ImGui::SetItemTooltip("Give the review a description to help describe what sounds you are adding or changing");

    ImGui::InputTextWithHint("Review URL", "https://domain.com/task", reviewUrlBuffer, textBufferSize);
    ImGui::SetItemTooltip("Add the Jira/ADO/HacknPlan task URL");

    if (ImGui::BeginCombo("Review Phase", get_review_phase_name(m_reviewData.m_reviewPhase)))
    {
        for (int i = 0; i < static_cast<int>(review_phase::num); ++i)
        {
            review_phase phase = static_cast<review_phase>(i);

            if (ImGui::Selectable(get_review_phase_name(phase)))
            {
                m_reviewData.m_reviewPhase = phase;
            }
        }

        ImGui::EndCombo();
    }
    ImGui::SetItemTooltip("Set the review phase. Useful for giving context as a temp sound does not need as much "
                          "vetting as a final pass sound");

    if (ImGui::BeginCombo("Review Quakity", get_review_quality_name(m_reviewData.m_reviewQuality)))
    {
        for (int i = 0; i < static_cast<int>(review_quality::num); ++i)
        {
            review_quality quality = static_cast<review_quality>(i);

            if (ImGui::Selectable(get_review_quality_name(quality)))
            {
                m_reviewData.m_reviewQuality = quality;
            }
        }

        ImGui::EndCombo();
    }
    ImGui::SetItemTooltip("Set the review quality. The higher the quality, the more the asset may need reviewing. Your "
                          "project may vary but roughly A == \"Industry Competitive\"");

    m_reviewData.m_reviewName = reviewNameBuffer;
    m_reviewData.m_reviewDescription = reviewDescriptionBuffer;
    m_reviewData.m_reviewTaskUrl     = reviewDescriptionBuffer;

    const bool setupValid = !m_reviewData.m_reviewName.empty();

    ImGui::BeginDisabled(!setupValid);

    if (ImGui::Button("Create"))
    {
        if (std::shared_ptr<workspace_manager> workspaceManager = get_app()->get_manager_by_class<workspace_manager>())
        {
            
        }

        close_popup();
    }

    ImGui::EndDisabled();

    ImGui::SameLine();

    if (ImGui::Button("Cancel"))
    {
        close_popup();
    }

    ImGui::SetWindowFontScale(1.0f);
}

auto workspace_widget::start_implementation() -> void
{
    ImGuiWindowClass windowClass;
    windowClass.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoTabBar;

    set_window_class(windowClass);
}

auto workspace_widget::render_window_implementation() -> void
{
    gluten::imgui::scoped_style noGaps(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

    render_left_toolbar();
    
    ImGui::SameLine();

    switch (m_activeView)
    {
        case workspace_widget::reviews_view:
            render_list();
            ImGui::SameLine();
            render_content();
            break;
        case workspace_widget::users_view:
            ImGui::TextUnformatted("Users");
            break;
        default:
            break;
    }
}

auto workspace_widget::render_list() -> void
{
    gluten::imgui::scoped_color backgroundColor(ImGuiCol_ChildBg, gluten::theme::carbon_g100::field02);
    gluten::imgui::scoped_color borderColor(ImGuiCol_Border, gluten::theme::carbon_g100::background);
    gluten::imgui::scoped_color separatorColor(ImGuiCol_Separator, gluten::theme::carbon_g100::background);

    static constexpr float buttonOffset = 20.0f;

    if (ImGui::BeginChild("ItemsPanel", ImVec2(itemListWidth, 0), ImGuiChildFlags_ResizeX))
    {
        gluten::element topToolbar(gluten::element::anchor_preset::stretch_top);
        topToolbar.get_element_anchor().maxOffset.y = leftToobarWidth;
        topToolbar.render_window();

        const bool listingProjects = !get_app()->get_manager_by_class<workspace_manager>()->has_selected_project();
        const bool listingReviews  = !listingProjects;

        gluten::text titleText(listingProjects ? "Projects" : "Reviews", ImVec2(0.5f, 0.5f), gluten::element::anchor_preset::center_middle);
        titleText
            .set_font(gluten::fonts::title)
            .set_element_content_font_size(gluten::g_baseFontSize * 2.0f)
            .render(topToolbar.get_element_rect());

        gluten::icon_button backButton("##BackButton", ICON_LC_ARROW_BIG_LEFT, gluten::fonts::regular_lucide_icons);
        backButton
            .set_icon_alignment(ImVec2(0.0f, 0.5f))
            .set_button_alignment(ImVec2(0.0f, 0.5f))
            .set_button_border(2.0f, 0.0f)
            .set_button_scale(1.2f)
            .set_element_background_color(gluten::theme::carbon_g100::field03)
            .set_element_hover_color(gluten::theme::carbon_g100::fieldHover03)
            .set_element_active_color(gluten::theme::carbon_g100::layerActive01)
            .set_element_anchor_preset(gluten::element::anchor_preset::left_middle)
            .set_element_content_scale(2.0f)
            .set_element_translation(ImVec2(buttonOffset, 0.0f));
        

        gluten::icon_button newButton("##NewButton", ICON_LC_PLUS, gluten::fonts::regular_lucide_icons);
        newButton
            .set_icon_alignment(ImVec2(1.0f, 0.5f))
            .set_button_alignment(ImVec2(1.0f, 0.5f))
            .set_button_border(2.0f, 0.0f)
            .set_button_scale(1.2f)
            .set_element_background_color(gluten::theme::carbon_g100::field03)
            .set_element_hover_color(gluten::theme::carbon_g100::fieldHover03)
            .set_element_active_color(gluten::theme::carbon_g100::layerActive01)
            .set_element_anchor_preset(gluten::element::anchor_preset::right_middle)
            .set_element_content_scale(2.0f)
            .set_element_translation(ImVec2(-buttonOffset, 0.0f));

        if (listingReviews)
        {
            if (backButton.render(topToolbar.get_element_rect()))
            {
                get_app()->get_manager_by_class<workspace_manager>()->select_project({});
            }
        }

        if (newButton.render(topToolbar.get_element_rect()))
        {
            if (listingProjects)
            {
                static std::shared_ptr<create_project_popup> createProjectPopup;
                createProjectPopup = add_child_widget<create_project_popup>(this);

                if (createProjectPopup)
                {
                    createProjectPopup->open_popup();
                }
            }
            else if (listingReviews)
            {
                static std::shared_ptr<create_review_popup> createReviewPopup;
                createReviewPopup = add_child_widget<create_review_popup>(this);

                if (createReviewPopup)
                {
                    createReviewPopup->open_popup();
                }
            }
        }

        ImGui::SetCursorPos(topToolbar.get_element_rect_local().GetBL());

        ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal, 2.0f);

        if (ImGui::BeginChild("ItemsList", ImVec2(0, 0), 0, ImGuiWindowFlags_AlwaysVerticalScrollbar))
        {
            gluten::layout itemsLayout(gluten::layout::layout_type::top_to_bottom, gluten::element::anchor_preset::stretch_full);
            itemsLayout.set_layout_spacing(2.0f);
            itemsLayout.render_window();

            if (listingProjects)
            {
                for (const auto& project : get_app()->get_manager_by_class<workspace_manager>()->get_projects())
                {
                    project_element projectElement(project->m_projectName, project->m_projectDescription, 2, 5);
                    if (itemsLayout.render_layout_element_pixels_vertical(&projectElement, 100.0f))
                    {
                        get_app()->get_manager_by_class<workspace_manager>()->select_project(project->m_projectName);
                    }
                }
            }
        }
        ImGui::EndChild();
    }
    ImGui::EndChild();
}

auto workspace_widget::render_content() -> void
{
    gluten::imgui::scoped_color backgroundColor(ImGuiCol_ChildBg, gluten::theme::carbon_g100::field03);

    if (ImGui::BeginChild("Content"))
    {
    }
    ImGui::EndChild();
}

auto workspace_widget::render_left_toolbar() -> void
{
    gluten::imgui::scoped_color toolbarBackgroundColor(ImGuiCol_ChildBg, gluten::theme::carbon_g100::field01);

    if (ImGui::BeginChild("LeftToolbar", ImVec2(leftToobarWidth, 0)))
    {
        gluten::layout buttonsLayout(gluten::layout::layout_type::top_to_bottom,
                                     gluten::element::anchor_preset::stretch_full);
        buttonsLayout.get_element_anchor().max.x += 0.1f;
        buttonsLayout.render_window();

        gluten::icon_button reviewsButton("##ReviewsButton", ICON_LC_CHART_NO_AXES_GANTT, gluten::fonts::regular_lucide_icons);
        reviewsButton
            .set_element_content_scale(3.0f)
            .set_element_hover_color(gluten::theme::carbon_g100::fieldHover01)
            .set_element_active_color(gluten::theme::carbon_g100::layerActive01)
            .set_element_active(m_activeView == reviews_view);

        gluten::icon_button usersButton("##UsersButton", ICON_LC_USERS, gluten::fonts::regular_lucide_icons);
        usersButton
            .set_element_content_scale(3.0f)
            .set_element_hover_color(gluten::theme::carbon_g100::fieldHover01)
            .set_element_active_color(gluten::theme::carbon_g100::layerActive01)
            .set_element_active(m_activeView == users_view);

        if (buttonsLayout.render_layout_element_pixels_vertical(&reviewsButton, leftToolbarButtonHeight))
        {
            m_activeView = reviews_view;
        }

        if (buttonsLayout.render_layout_element_pixels_vertical(&usersButton, leftToolbarButtonHeight))
        {
            m_activeView = users_view;
        }
    }
    ImGui::EndChild();
}

auto workspace_widget::render_menu_implementation() -> void
{
    if (ImGui::BeginMenu(s_fileMenuName))
    {
        if (ImGui::MenuItem("Close Workspace"))
        {
            get_app()->get_manager_by_class<workspace_manager>()->close_workspace();
        }
        ImGui::EndMenu();
    }
}