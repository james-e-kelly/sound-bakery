#include "workspace_widget.h"

#include "app/review_app.h"
#include "data/activity_data.h"
#include "managers/workspace_manager.h"
#include "elements/file_drop_element.h"
#include "elements/project_element.h"
#include "elements/review_element.h"
#include "elements/inline_user_display_element.h"
#include "widgets/create_project_popup.h"
#include "widgets/create_review_popup.h"
#include "widgets/update_review_popup.h"

namespace
{
    constexpr float leftToobarWidth = gluten::g_baseFontSize * 5.0f;
    constexpr float leftToolbarButtonHeight     = leftToobarWidth;
    constexpr float topHeaderHeight             = leftToobarWidth;
    constexpr float leftToolbarHalfButtonHeight = leftToolbarButtonHeight / 2.0f;
    constexpr float descriptionBoxHeight        = topHeaderHeight * 2.0f;

    constexpr float itemListWidth = leftToobarWidth * 5.0f;
    constexpr float rightPanelWidth = leftToobarWidth * 4.0f;
}

auto workspace_widget::start_implementation() -> void
{
    ImGuiWindowClass windowClass;
    windowClass.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoTabBar;

    set_window_class(windowClass);

    m_workspaceManager = get_app()->get_manager_by_class<workspace_manager>();
}

auto workspace_widget::render_window_implementation() -> void
{
    gluten::imgui::scoped_style noGaps(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

    render_left_toolbar();
    
    ImGui::SameLine();

    switch (m_activeView)
    {
        case workspace_widget::reviews_view:
        case workspace_widget::users_view:
            render_list();
            ImGui::SameLine();
            render_content();
            break;
        default:
            break;
    }
}

auto workspace_widget::render_list() -> void
{
    gluten::imgui::scoped_color backgroundColor(ImGuiCol_ChildBg, gluten::theme::carbon_g100::layer02);
    gluten::imgui::scoped_color borderColor(ImGuiCol_Border, gluten::theme::carbon_g100::background);
    gluten::imgui::scoped_color separatorColor(ImGuiCol_Separator, gluten::theme::carbon_g100::background);

    static constexpr float buttonOffset = 20.0f;

    std::shared_ptr<workspace_manager> workspaceManager = m_workspaceManager.lock();

    if (!workspaceManager)
    {
        return;
    }

    if (ImGui::BeginChild("ItemsPanel", ImVec2(itemListWidth, 0), ImGuiChildFlags_ResizeX))
    {
        gluten::element topToolbar(gluten::element::anchor_preset::stretch_top);
        topToolbar.get_element_anchor().maxOffset.y = leftToobarWidth;
        topToolbar.render_window();
        
        const bool listingProjects = !workspaceManager->has_selected_project();
        const bool listingReviews  = !listingProjects;

        std::string titleString;

        switch (m_activeView)
        {
            case workspace_widget::reviews_view:
                titleString = listingProjects ? "Projects" : "Reviews";
                break;
            case workspace_widget::users_view:
                titleString = "Users";
                break;
            case workspace_widget::settings_view:
                break;
            default:
                break;
        }

        gluten::text titleText(titleString, ImVec2(0.5f, 0.5f), gluten::element::anchor_preset::center_middle);
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
                workspaceManager->select_project({});
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

            switch (m_activeView)
            {
                case workspace_widget::reviews_view:
                    if (listingProjects)
                    {
                        for (const auto& project : workspaceManager->get_projects())
                        {
                            project_element projectElement(project.m_projectName, project.m_projectDescription, 2, 5);
                            if (itemsLayout.render_layout_element_pixels_vertical(&projectElement,
                                                                                  leftToolbarButtonHeight))
                            {
                                workspaceManager->select_project(project.m_projectName);
                            }
                        }
                    }
                    else if (listingReviews)
                    {
                        for (const auto& review : workspaceManager->get_all_reviews())
                        {
                            review_element reviewElement(review);
                            if (itemsLayout.render_layout_element_pixels_vertical(&reviewElement,
                                                                                  leftToolbarButtonHeight))
                            {
                                workspaceManager->select_review(review.m_reviewId);
                            }
                        }
                    }
                    break;
                case workspace_widget::users_view:
                    for (const auto& user : workspaceManager->get_all_users())
                    {
                        gluten::text userText(user.m_displayName);
                        if (itemsLayout.render_layout_element_pixels_vertical(&userText, leftToolbarButtonHeight))
                        {
                        }
                    }
                    break;
                case workspace_widget::settings_view:
                    break;
                default:
                    break;
            }
        }
        ImGui::EndChild();
    }
    ImGui::EndChild();
}

auto workspace_widget::render_content() -> void
{
    gluten::imgui::scoped_color backgroundColor(ImGuiCol_ChildBg, gluten::theme::carbon_g100::layer03);
    gluten::imgui::scoped_color borderColor(ImGuiCol_Border, gluten::theme::carbon_g100::borderStrong02);
    gluten::imgui::scoped_color tabBg(ImGuiCol_Tab, gluten::theme::carbon_g100::field03);
    gluten::imgui::scoped_color tabSelectedBg(ImGuiCol_TabActive, gluten::theme::carbon_g100::layerAccentActive03);
    gluten::imgui::scoped_color tabHoverdBg(ImGuiCol_TabHovered, gluten::theme::carbon_g100::layerHover03);
    gluten::imgui::scoped_color frameBg(ImGuiCol_FrameBg, gluten::theme::carbon_g100::layer02);
    gluten::imgui::scoped_color frameHoveredBg(ImGuiCol_FrameBgHovered, gluten::theme::carbon_g100::layerHover02);
    gluten::imgui::scoped_font iconFont(gluten::app::get()->get_font(gluten::fonts::regular_lucide_icons));

    std::shared_ptr<workspace_manager> workspaceManager = m_workspaceManager.lock();

    if (ImGui::BeginChild("Content") && workspaceManager)
    {
        const project_data& selectedProject = workspaceManager->get_selected_project();
        const review_data& selectedReview   = workspaceManager->get_selected_review();

        gluten::layout verticalLayout(gluten::layout::layout_type::top_to_bottom, gluten::element::anchor_preset::stretch_full);
        verticalLayout.render_window();

        gluten::background titleBarBackground;
        titleBarBackground.set_element_background_color(gluten::theme::carbon_g100::fieldHover02);
        verticalLayout.render_layout_element_pixels_vertical(&titleBarBackground, topHeaderHeight);

        std::string breadcrumbString = workspaceManager->get_workspace_name();

        if (selectedProject.m_id != 0)
        {
            breadcrumbString += " / " + selectedProject.m_projectName;
        }

        if (selectedReview.m_reviewId != 0)
        {
            breadcrumbString += " / " + selectedReview.m_reviewName;
        }

        gluten::text breadcrumbText(breadcrumbString.c_str(), ImVec2(0, -0.75f), gluten::element::anchor_preset::left_top);
        breadcrumbText
            .set_element_content_font_size(gluten::g_baseFontSize * 2.0f)
            .set_element_translation(ImVec2(5, 0.0f));
        breadcrumbText.render(titleBarBackground.get_element_rect());

        logged_in_user_element loggedInUserAvatar(m_userSettings->m_loggedInUser.m_email);
        loggedInUserAvatar.set_element_anchor_preset(gluten::anchor_preset::stretch_right);
        loggedInUserAvatar.get_element_anchor().minOffset.x = -titleBarBackground.get_element_rect().GetHeight();
        loggedInUserAvatar.render(titleBarBackground.get_element_rect());

        gluten::layout contentAndRightPanelLayout(gluten::layout::layout_type::right_to_left, gluten::element::anchor_preset::stretch_full);
        verticalLayout.render_layout_element_remaining(&contentAndRightPanelLayout);

        gluten::layout rightPanelBackground(gluten::layout::layout_type::top_to_bottom, gluten::element::anchor_preset::stretch_full);
        rightPanelBackground
            .set_layout_spacing(ImGui::GetStyle().FramePadding.y)
            .set_element_background_color(gluten::theme::carbon_g100::fieldHover03);
        contentAndRightPanelLayout.render_layout_element_pixels_horizontal(&rightPanelBackground, rightPanelWidth);

        gluten::background mainContentParent;
        contentAndRightPanelLayout.render_layout_element_remaining(&mainContentParent);

        gluten::layout contentVerticalLayout(gluten::layout::layout_type::top_to_bottom, gluten::element::anchor_preset::stretch_full);
        contentVerticalLayout.set_layout_spacing(10.0f);
        contentVerticalLayout.render(mainContentParent.get_element_rect());

        if (selectedReview.m_reviewId)
        {
            {
                gluten::collapsing_header reviewersHeader("Reviewers");

                if (rightPanelBackground.render_layout_element_pixels_vertical(&reviewersHeader, 50.0f))
                {
                    inline_user_display_element user("James Kelly", "james@jameskelly.audio");
                    inline_user_display_element user2("Jack Tysoe", "jtysoe@climaxstudios.com");
                    inline_user_display_element user3("Sam Packer", "spacker@climaxstudios.com");
                    inline_user_display_element user4("Rory Burcham", "rburcham@climaxstudios.com");
                    inline_user_display_element user5("Andrada Vaduvoiu", "avaduvoiu@climaxstudios.com");
                    inline_user_display_element user6("James", "jkelly@climaxstudios.com");
                    rightPanelBackground.render_layout_element_pixels_vertical(&user, 50.0f);
                    rightPanelBackground.render_layout_element_pixels_vertical(&user2, 50.0f);
                    rightPanelBackground.render_layout_element_pixels_vertical(&user3, 50.0f);
                    rightPanelBackground.render_layout_element_pixels_vertical(&user4, 50.0f);
                    rightPanelBackground.render_layout_element_pixels_vertical(&user5, 50.0f);
                    rightPanelBackground.render_layout_element_pixels_vertical(&user6, 50.0f);
                }

                gluten::collapsing_header testsHeader("Tests");

                if (rightPanelBackground.render_layout_element_pixels_vertical(&testsHeader, 50.0f))
                {
                }
            }

            gluten::background descriptionBox;
            contentVerticalLayout.render_layout_element_pixels_vertical(&descriptionBox, 200.0f);

            gluten::layout innerDescriptionBox;
            innerDescriptionBox
                .set_layout_type(gluten::layout::layout_type::top_to_bottom)
                .set_element_anchor_preset(gluten::element::anchor_preset::stretch_full)
                .set_element_padding(ImGui::GetStyle().FramePadding);
            innerDescriptionBox.render(descriptionBox.get_element_rect());

            gluten::text titleText(fmt::format("{}", selectedReview.m_reviewName).c_str(), ImVec2(0.0f, 0.0f), gluten::element::anchor_preset::stretch_full);
            titleText
                .set_url(selectedReview.m_reviewTaskUrl)
                .set_font(gluten::fonts::title_lucide_icons)
                .set_element_content_font_size(gluten::g_baseFontSize * 1.5f);
            innerDescriptionBox.render_layout_element_percent_vertical(&titleText, 0.3f);

            gluten::text descriptionText(fmt::format("{} {}", ICON_LC_MESSAGE_CIRCLE, selectedReview.m_reviewDescription).c_str(), ImVec2(0.0f, 0.0f), gluten::element::anchor_preset::stretch_full);
            descriptionText
                .set_font(gluten::fonts::regular_lucide_icons)
                .set_element_content_font_size(gluten::g_baseFontSize * 1.3f);
            innerDescriptionBox.render_layout_element_percent_vertical(&descriptionText, 0.2f);

            gluten::text phaseText(fmt::format("{} {}", ICON_LC_WORKFLOW, get_review_phase_string(selectedReview.m_reviewPhase)).c_str(), ImVec2(0.0f, 0.0f), gluten::element::anchor_preset::stretch_full);
            phaseText.set_font(gluten::fonts::regular_lucide_icons);
            phaseText.set_element_content_font_size(gluten::g_baseFontSize * 1.3f);
            innerDescriptionBox.render_layout_element_percent_vertical(&phaseText, 0.2f);

            gluten::text qualityText(fmt::format("{} {}", ICON_LC_AWARD, get_review_quality_string(selectedReview.m_reviewQuality)).c_str(), ImVec2(0.0f, 0.0f), gluten::element::anchor_preset::stretch_full);
            qualityText.set_font(gluten::fonts::regular_lucide_icons);
            qualityText.set_element_content_font_size(gluten::g_baseFontSize * 1.3f);
            innerDescriptionBox.render_layout_element_percent_vertical(&qualityText, 0.2f);

            {
                gluten::imgui::scoped_color frameProgressBg(ImGuiCol_FrameBg, gluten::theme::carbon_g100::layer03);

                gluten::layout scrutinyLayout(gluten::layout::layout_type::left_to_right);
                scrutinyLayout.set_element_anchor_preset(gluten::element::anchor_preset::stretch_full);
                innerDescriptionBox.render_layout_element_percent_vertical(&scrutinyLayout, 0.2f);

                constexpr float max = ((int)review_phase::num) * ((int)review_quality::num);
                constexpr float third = max * 0.33f;
                constexpr float twoThirds = max * 0.66f;
                const float value   = ((int)selectedReview.m_reviewPhase + 1) * ((int)selectedReview.m_reviewQuality + 1);
                const float fraction = value / max;

                gluten::imgui::scoped_color progressBarColor(ImGuiCol_PlotHistogram, value >= twoThirds ? gluten::theme::red60 : value >= third ? gluten::theme::yellow60 : gluten::theme::green60);

                ImGui::ProgressBar(fraction, scrutinyLayout.get_element_rect().GetSize(), "Scrutiny");
            }

            gluten::button descriptionEditButton("Edit " ICON_LC_PENCIL_LINE, false, gluten::element::anchor_preset::right_top);

            descriptionEditButton.set_element_alignment(ImVec2(1.0f, -0.0f));

            if (descriptionEditButton.render(innerDescriptionBox.get_element_rect()))
            {
                static std::shared_ptr<update_review_popup> updateProjectPopup;
                updateProjectPopup = add_child_widget<update_review_popup>(false);

                if (updateProjectPopup)
                {
                    updateProjectPopup->set_review_data(selectedReview);
                    updateProjectPopup->open_popup();
                }
            }

            gluten::background remaining;
            remaining.set_element_frame_padding();
            remaining.get_element_anchor().maxOffset.y -= 20.0f;
            contentVerticalLayout.render_layout_element_remaining(&remaining);

            if (ImGui::BeginChild("ReviewContent", remaining.get_element_rect().GetSize()))
            {
                if (ImGui::BeginTabBar("Tabs"))
                {
                    if (ImGui::BeginTabItem("Files"))
                    {
                        ImGui::Dummy(ImVec2(0.0f, ImGui::GetStyle().FramePadding.y));

                        if (ImGui::BeginCombo("Review Version", "#1"))
                        {
                            ImGui::Selectable("#1");
                            ImGui::Selectable("#2");

                            ImGui::EndCombo();
                        }

                        ImGui::EndTabItem();
                    }

                    if (ImGui::BeginTabItem("Comments"))
                    {
                        ImGui::Dummy(ImVec2(0.0f, ImGui::GetStyle().FramePadding.y));

                        const std::vector<comment_data>& comments = workspaceManager->get_all_comments_for_review(selectedReview.m_reviewId);
                        
                        static bool newCommentOpen = false;

                        ImGui::BeginDisabled(newCommentOpen);
                        if (ImGui::Button(ICON_LC_PLUS))
                        {
                            newCommentOpen = true;
                        }
                        ImGui::EndDisabled();

                        constexpr std::size_t commentBufferSize = 2045;
                        static char buffer[commentBufferSize] = {0};

                        if (newCommentOpen)
                        {
                            ImGui::Dummy(ImVec2(0.0f, ImGui::GetStyle().FramePadding.y));

                            bool wantsToAdd = false;
                            
                            if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Enter))
                            {
                                wantsToAdd = true;
                            }

                            ImGui::InputTextMultiline("Comment", buffer, commentBufferSize, ImVec2(ImGui::GetWindowSize().x, 100.0f));

                            ImGui::Dummy(ImVec2(0.0f, ImGui::GetStyle().FramePadding.y));

                            wantsToAdd |= ImGui::Button("Add");

                            if (wantsToAdd)
                            {
                                new_comment_data newComment = {0};
                                newComment.m_reviewId       = selectedReview.m_reviewId;
                                newComment.m_comment        = buffer;
                                newComment.m_userId         = 1;

                                workspaceManager->create_comment(newComment);

                                buffer[0]      = '\0';
                                newCommentOpen = false;
                            }

                            ImGui::SameLine(0.0f, 8.0f);

                            if (ImGui::Button("Cancel"))
                            {
                                buffer[0]      = '\0';
                                newCommentOpen = false;
                            }
                        }

                        ImGui::Dummy(ImVec2(0.0f, ImGui::GetStyle().FramePadding.y));

                        for (const auto& comment : comments)
                        {
                            ImGui::TextWrapped(comment.m_comment.c_str());

                            bool requestBreak = false;

                            if (ImGui::BeginPopupContextItem(comment.m_comment.c_str()))
                            {
                                if (ImGui::Selectable("Copy"))
                                {
                                    ImGui::SetClipboardText(comment.m_comment.c_str());
                                }

                                if (ImGui::Selectable("Delete"))
                                {
                                    workspaceManager->delete_comment(comment.m_commentId);
                                    requestBreak = true;
                                }
                                ImGui::EndPopup();
                            }
                            ImGui::Dummy(ImVec2(0.0f, ImGui::GetStyle().FramePadding.y));

                            if (requestBreak)
                            {
                                break;
                            }
                        }

                        ImGui::EndTabItem();
                    }

                    if (ImGui::BeginTabItem("Activity"))
                    {
                        const std::vector<activity_data>& reviewsActivity = workspaceManager->get_all_activity_for_review(selectedReview.m_reviewId);

                        for (const auto& iter : reviewsActivity)
                        {
                            std::istringstream in(iter.m_activityTimestamp);
                            std::chrono::sys_seconds tp;
                            in >> std::chrono::parse("%F %T", tp);

                            auto now  = std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
                            auto diff = now - tp;

                            using namespace std::literals::chrono_literals;

                            std::string activityTimeText;

                            if (diff < 1min)
                            {
                                activityTimeText = "less than 1 minute ago";
                            }
                            else if (diff <= 60min)
                            {
                                activityTimeText = std::to_string(duration_cast<std::chrono::minutes>(diff).count()) + " minutes ago";
                            }
                            else if (diff < 24h)
                            {
                                activityTimeText = std::to_string(duration_cast<std::chrono::hours>(diff).count()) + " hours ago";
                            }
                            else
                            {
                                auto days = duration_cast<std::chrono::duration<int, std::ratio<86400>>>(diff).count();
                                activityTimeText = std::to_string(days) + " day" + (days > 1 ? "s" : "") + " ago";
                            }

                            ImGui::Dummy(ImVec2(0, ImGui::GetStyle().FramePadding.y));
                            ImGui::Text(fmt::format("{} {}", iter.m_activityText, activityTimeText).c_str());
                        }

                        ImGui::EndTabItem();
                    }

                    ImGui::EndTabBar();
                }
            }
            ImGui::EndChild();
        }
    }
    ImGui::EndChild();
}

auto workspace_widget::render_left_toolbar() -> void
{
    gluten::imgui::scoped_color toolbarBackgroundColor(ImGuiCol_ChildBg, gluten::theme::carbon_g100::layer01);

    if (ImGui::BeginChild("LeftToolbar", ImVec2(leftToobarWidth, 0)))
    {
        gluten::layout buttonsLayout(gluten::layout::layout_type::top_to_bottom,
                                     gluten::element::anchor_preset::stretch_full);
        buttonsLayout.get_element_anchor().max.x += 0.1f;
        buttonsLayout.render_window();

        gluten::icon_button reviewsButton("##ReviewsButton", ICON_LC_CHART_NO_AXES_GANTT, gluten::fonts::regular_lucide_icons);
        reviewsButton
            .set_element_content_font_size(gluten::g_baseIconFontSize * (leftToolbarButtonHeight / gluten::g_baseIconFontSize) / 1.75f)
            .set_element_hover_color(gluten::theme::carbon_g100::fieldHover01)
            .set_element_active_color(gluten::theme::carbon_g100::layerActive01)
            .set_element_active(m_activeView == reviews_view);

        gluten::icon_button usersButton("##UsersButton", ICON_LC_USERS, gluten::fonts::regular_lucide_icons);
        usersButton
            .set_element_content_font_size(gluten::g_baseIconFontSize * (leftToolbarButtonHeight / gluten::g_baseIconFontSize) / 1.75f)
            .set_element_hover_color(gluten::theme::carbon_g100::fieldHover01)
            .set_element_active_color(gluten::theme::carbon_g100::layerActive01)
            .set_element_active(m_activeView == users_view);

        gluten::icon_button settingsButton("##SettingsButton", ICON_LC_SETTINGS, gluten::fonts::regular_lucide_icons);
        settingsButton
            .set_element_content_font_size(gluten::g_baseIconFontSize * (leftToolbarButtonHeight / gluten::g_baseIconFontSize) / 1.75f)
            .set_element_hover_color(gluten::theme::carbon_g100::fieldHover01)
            .set_element_active_color(gluten::theme::carbon_g100::layerActive01)
            .set_element_active(m_activeView == settings_view);

        if (buttonsLayout.render_layout_element_pixels_vertical(&reviewsButton, leftToolbarButtonHeight))
        {
            m_activeView = reviews_view;
        }

        if (buttonsLayout.render_layout_element_pixels_vertical(&usersButton, leftToolbarButtonHeight))
        {
            m_activeView = users_view;
        }

        if (buttonsLayout.render_layout_element_pixels_vertical(&settingsButton, leftToolbarButtonHeight))
        {
            m_activeView = settings_view;
        }
    }
    ImGui::EndChild();
}

auto workspace_widget::render_menu_implementation() -> void
{
    if (ImGui::BeginMenu(s_fileMenuName))
    {
        if (ImGui::MenuItem("Create New User..."))
        {
            get_app()->get_manager_by_class<workspace_manager>()->open_create_user_popup();
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Close Workspace"))
        {
            get_app()->get_manager_by_class<workspace_manager>()->close_workspace();
        }

        if (ImGui::MenuItem("Logout..."))
        {
            get_app()->get_manager_by_class<workspace_manager>()->logout();
        }
        ImGui::EndMenu();
    }
}