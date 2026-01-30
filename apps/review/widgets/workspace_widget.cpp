#include "workspace_widget.h"

#include "app/review_app.h"
#include "data/activity_data.h"
#include "managers/workspace_manager.h"
#include "elements/file_drop_element.h"
#include "elements/project_element.h"
#include "elements/review_element.h"
#include "elements/user_element.h"
#include "elements/inline_user_display_element.h"
#include "elements/video_element.h"
#include "elements/audio_element.h"
#include "subsystems/video_subsystem.h"
#include "widgets/create_comment_popup.h"
#include "widgets/create_project_popup.h"
#include "widgets/create_review_popup.h"
#include "widgets/update_review_popup.h"
#include "widgets/edit_reviewers_popup.h"

namespace
{
    constexpr float leftToobarWidth = gluten::g_baseFontSize * 4.5f;
    constexpr float leftToolbarButtonHeight     = leftToobarWidth;
    constexpr float topHeaderHeight             = leftToobarWidth;
    constexpr float leftToolbarHalfButtonHeight = leftToolbarButtonHeight / 2.0f;
    constexpr float descriptionBoxHeight        = topHeaderHeight * 2.0f;

    constexpr float g_itemListWidth = leftToobarWidth * 5.0f;
    constexpr float g_rightPanelWidth = leftToobarWidth * 1.0f;
}

auto workspace_widget::start_implementation() -> void
{
    ImGuiWindowClass windowClass;
    windowClass.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoTabBar;

    set_window_class(windowClass);

    m_workspaceManager = get_app()->get_manager_by_class<workspace_manager>();

    topContentBarBackground
        .set_element_background_color(gluten::theme::carbon_g100::layer01)
        .set_element_border(1.0f, 0.0f)
        .set_element_anchor_preset(gluten::anchor_preset::stretch_top);
    topContentBarBackground.get_element_anchor().maxOffset.y += topHeaderHeight;

    buttonsLayout.get_element_anchor().max.x += 0.1f;
    breadcrumbText.set_element_content_font_size(gluten::g_baseFontSize * 2.0f)
        .set_element_translation(ImVec2(5, 0.0f));

    rightPanelLayout.set_layout_spacing(ImGui::GetStyle().FramePadding.y)
        .set_element_background_color(gluten::theme::carbon_g100::layer01);
    rightPanelLayout.get_element_anchor().minOffset.y += topHeaderHeight;
    rightPanelLayout.get_element_anchor().maxOffset.x += g_rightPanelWidth - 2.0f;
    rightPanelLayout.set_element_translation(ImVec2(-g_rightPanelWidth, 0.0f));

    editReviewersButton.set_element_alignment(ImVec2(1.0f, -0.1f));
    editReviewersButton.set_element_translation(ImVec2(-ImGui::GetStyle().FramePadding.x, 0.0f));
    descriptionBoxLayout.set_layout_type(gluten::layout::layout_type::top_to_bottom)
        .set_element_anchor_preset(gluten::element::anchor_preset::stretch_full);
    titleText
        .set_font(gluten::fonts::title_lucide_icons)
        .set_element_content_font_size(gluten::g_baseFontSize * 1.5f);
    descriptionText.set_font(gluten::fonts::regular_lucide_icons)
        .set_element_content_font_size(gluten::g_baseFontSize * 1.3f);
    qualityText.set_font(gluten::fonts::regular_lucide_icons);
    qualityText.set_element_content_font_size(gluten::g_baseFontSize * 1.3f);
    votesText.set_font(gluten::fonts::regular_lucide_icons);
    votesText.set_element_content_font_size(gluten::g_baseFontSize * 1.3f);
    votesIconText.set_font(gluten::fonts::regular_lucide_icons);
    votesIconText.set_element_content_font_size(gluten::g_baseFontSize * 1.3f);
    phaseText.set_font(gluten::fonts::regular_lucide_icons);
    phaseText.set_element_content_font_size(gluten::g_baseFontSize * 1.3f);
    scrutinyLayout.set_element_anchor_preset(gluten::element::anchor_preset::stretch_full);
    descriptionEditButton.set_element_alignment(ImVec2(1.0f, -0.0f));
    m_reviewFilesLayout.set_layout_spacing(20.0f);

    m_descriptionBoxButtonsLayout.set_layout_spacing(8.0f);

    listItemsTitle.set_font(gluten::fonts::title).set_element_content_font_size(gluten::g_baseFontSize * 2.0f);

    constexpr float listItemButtonsOffset = 20.0f;

    listItemsBackButton.set_element_border(2.0f, 0.0f)
        .set_element_scale(1.1f)
        .set_element_background_color(gluten::theme::carbon_g100::field03)
        .set_element_hover_color(gluten::theme::carbon_g100::fieldHover03)
        .set_element_active_color(gluten::theme::carbon_g100::layerActive01)
        .set_element_anchor_preset(gluten::element::anchor_preset::left_middle)
        .set_element_content_scale(2.0f)  // Icon size
        .set_element_alignment(ImVec2(0.5f, 0.5f))
        .set_element_translation(ImVec2(listItemButtonsOffset, 0.0f));

    listItemsNewButton.set_element_border(2.0f, 0.0f)
        .set_element_scale(1.1f)
        .set_element_background_color(gluten::theme::carbon_g100::field03)
        .set_element_hover_color(gluten::theme::carbon_g100::fieldHover03)
        .set_element_active_color(gluten::theme::carbon_g100::layerActive01)
        .set_element_anchor_preset(gluten::element::anchor_preset::right_middle)
        .set_element_content_scale(2.0f)  // Icon size
        .set_element_alignment(ImVec2(0.5f, 0.5f))
        .set_element_translation(ImVec2(-listItemButtonsOffset, 0.0f));
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
            render_list_elements_panel();
            ImGui::SameLine();
            render_content();
            break;
        case workspace_widget::settings_view:
            render_settings();
            break;
        default:
            break;
    }
}

auto workspace_widget::render_list_elements_panel() -> void
{
    gluten::imgui::scoped_color backgroundColor(ImGuiCol_ChildBg, gluten::theme::carbon_g100::layer02);
    gluten::imgui::scoped_color borderColor(ImGuiCol_Border, gluten::theme::carbon_g100::background);
    gluten::imgui::scoped_color separatorColor(ImGuiCol_Separator, gluten::theme::carbon_g100::background);

    std::shared_ptr<workspace_manager> workspaceManager = m_workspaceManager.lock();

    if (!workspaceManager)
    {
        return;
    }

    if (ImGui::BeginChild("ItemsPanel", ImVec2(g_itemListWidth, 0), ImGuiChildFlags_ResizeX))
    {
        gluten::element topToolbar(gluten::element::anchor_preset::stretch_top);
        topToolbar.get_element_anchor().maxOffset.y = leftToobarWidth;
        topToolbar.render_window();
        
        const bool listingProjects = !workspaceManager->has_selected_project();
        const bool listingReviews  = !listingProjects;

        switch (m_activeView)
        {
            case workspace_widget::reviews_view:
                listItemsTitle.set_text(listingProjects ? "Projects" : "Reviews");
                break;
            case workspace_widget::users_view:
                listItemsTitle.set_text("Users");
                break;
            case workspace_widget::settings_view:
                break;
            default:
                break;
        }

        listItemsTitle.render(topToolbar.get_element_rect());

        if (listingReviews)
        {
            if (listItemsBackButton.render(topToolbar.get_element_rect()))
            {
                workspaceManager->select_project({});
            }
        }

        if ((m_activeView == active_view::reviews_view || m_activeView == active_view::users_view) && m_userSettings->m_loggedInUser.m_privileges > user_privileges::guest)
        {
            if (listItemsNewButton.render(topToolbar.get_element_rect()))
            {
                if (m_activeView == active_view::users_view)
                {
                    workspaceManager->open_create_user_popup();
                }
                else
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
                        createReviewPopup = add_child_widget<create_review_popup>(this);

                        if (createReviewPopup)
                        {
                            createReviewPopup->open_popup();
                        }
                    }
                }
            }
        }

        ImGui::SetCursorPos(topToolbar.get_element_rect_local().GetBL());

        ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal, 2.0f);

        render_list_panel_elements(listingProjects, workspaceManager,
                                   listingReviews);
    }
    ImGui::EndChild();
}

auto workspace_widget::render_list_panel_elements(const bool listingProjects, std::shared_ptr<workspace_manager>& workspaceManager, const bool listingReviews) -> void
{
    gluten::imgui::scoped_color backgroundColor(
        ImGuiCol_ChildBg, gluten::theme::carbon_g100::layer02);

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
                    const auto& allProjects = workspaceManager->get_all_projects();

                    if (allProjects.has_data())
                    {
                        for (const auto& project : allProjects.m_cache)
                        {
                            project_element projectElement(project.m_projectName, project.m_projectDescription, project.m_openReviews, project.m_closedReviews, project.m_archivedReviews);
                            if (itemsLayout.render_layout_element_pixels_vertical(&projectElement,leftToolbarButtonHeight))
                            {
                                workspaceManager->select_project(project.m_projectName);
                                m_editReviewers.set_project_id(project.m_id);
                            }
                        }
                    }
                    else
                    {
                        gluten::loading_spinner loadingSpinner;
                        itemsLayout.render_layout_element_pixels_vertical(&loadingSpinner, leftToolbarButtonHeight);
                    }
                }
                else if (listingReviews)
                {
                    const auto& allReviews = workspaceManager->get_all_reviews();

                    gluten::collapsing_header openReviewsHeader("Open", true);
                    gluten::collapsing_header closedReviewsHeader("Closed", false);
                    gluten::collapsing_header archivedReviewsHeader("Archived", false);

                    gluten::imgui::scoped_color_stack headerColors(ImGuiCol_Header, gluten::theme::carbon_g100::layerAccent03, ImGuiCol_HeaderHovered, gluten::theme::carbon_g100::layerAccentHover03);

                    auto render_reviews = [&allReviews, &workspaceManager, &itemsLayout](review_status statusToRender)
                        {
                            if (allReviews.has_data())
                            {
                                for (const auto& review : allReviews.m_cache)
                                {
                                    if (review.m_reviewStatus == statusToRender)
                                    {
                                        review_element reviewElement(review);
                                        if (itemsLayout.render_layout_element_pixels_vertical(&reviewElement, leftToolbarButtonHeight))
                                        {
                                            workspaceManager->select_review(review.m_reviewId);
                                        }

                                    }
                                }
                                itemsLayout.render_vertical_spacer(leftToolbarHalfButtonHeight / 2.0f);
                            }
                            else
                            {
                                gluten::loading_spinner loadingSpinner;
                                itemsLayout.render_layout_element_pixels_vertical(&loadingSpinner, leftToolbarButtonHeight);
                            }
                        };

                    if (itemsLayout.render_layout_element_pixels_vertical(&openReviewsHeader, leftToolbarHalfButtonHeight))
                    {
                        render_reviews(review_status::open);
                    }

                    if (itemsLayout.render_layout_element_pixels_vertical(&closedReviewsHeader, leftToolbarHalfButtonHeight))
                    {
                        render_reviews(review_status::closed);
                    }

                    if (itemsLayout.render_layout_element_pixels_vertical(&archivedReviewsHeader, leftToolbarHalfButtonHeight))
                    {
                        render_reviews(review_status::archived);
                    }
                }
                break;
            case workspace_widget::users_view:
            {
                const auto& allUsers = workspaceManager->get_all_users();

                if (allUsers.has_data())
                {
                    for (const auto& user : allUsers.m_cache)
                    {
                        user_element userElement(user);
                        if (itemsLayout.render_layout_element_pixels_vertical(&userElement, leftToolbarButtonHeight))
                        {
                            workspaceManager->select_user(user.m_email);
                        }
                    }
                }
                else
                {
                    gluten::loading_spinner loadingSpinner;
                    itemsLayout.render_layout_element_pixels_vertical(&loadingSpinner, leftToolbarButtonHeight);
                }
                break;
            }
            case workspace_widget::settings_view:
                break;
            default:
                break;
        }
    }
    ImGui::EndChild();
}

auto workspace_widget::render_content() -> void
{
    gluten::imgui::scoped_color backgroundColor(ImGuiCol_ChildBg, gluten::theme::carbon_g100::layer01);
    gluten::imgui::scoped_color borderColor(ImGuiCol_Border, gluten::theme::carbon_g100::borderStrong02);
    gluten::imgui::scoped_color tabBg(ImGuiCol_Tab, gluten::theme::carbon_g100::field01);
    gluten::imgui::scoped_color tabSelectedBg(ImGuiCol_TabActive, gluten::theme::carbon_g100::layerAccentActive01);
    gluten::imgui::scoped_color tabHoverdBg(ImGuiCol_TabHovered, gluten::theme::carbon_g100::layerHover01);
    gluten::imgui::scoped_color frameBg(ImGuiCol_FrameBg, gluten::theme::carbon_g100::layer02);
    gluten::imgui::scoped_color frameHoveredBg(ImGuiCol_FrameBgHovered, gluten::theme::carbon_g100::layerHover01);
    gluten::imgui::scoped_color header(ImGuiCol_Header, gluten::theme::carbon_g100::layer02);
    gluten::imgui::scoped_color headerBg(ImGuiCol_HeaderHovered, gluten::theme::carbon_g100::layerHover01);
    gluten::imgui::scoped_font iconFont(gluten::app::get()->get_font(gluten::fonts::regular_lucide_icons));

    std::shared_ptr<workspace_manager> workspaceManager = m_workspaceManager.lock();

    if (ImGui::BeginChild("ContentContainer", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar))
    {
        const project_data& selectedProject = workspaceManager->get_selected_project();
        const review_data& selectedReview   = workspaceManager->get_selected_review();
        const user_data& selectedUser       = workspaceManager->get_selected_user();
        
        render_top_content_bar(workspaceManager, selectedProject, selectedReview);
        render_right_panel(workspaceManager, selectedReview, selectedUser);

        ImGui::SetCursorScreenPos(topContentBarBackground.get_element_rect().GetBL());

        gluten::imgui::scoped_style childPadding(ImGuiStyleVar_WindowPadding, selectedReview.m_reviewId ? ImVec2(8.0f, 8.0f) : ImVec2(0.0f, 0.0f));

        if (ImGui::BeginChild("Content",
                              ImVec2(ImGui::GetWindowWidth() - g_rightPanelWidth,
                                     ImGui::GetWindowHeight() - (topHeaderHeight * 1.25f)),
                              ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysUseWindowPadding, 0))
        {
            ImRect windowRect = ImGui::GetCurrentWindow()->WorkRect;
            windowRect.Max.y  = windowRect.Min.y + 10.0f;

            contentVerticalLayout.render(windowRect);

            if (selectedReview.m_reviewId)
            {
                render_review_description(selectedReview);

                const float extraDescriptionBoxSize = descriptionBoxLayout.get_remaining_layout_size().y;

                if (extraDescriptionBoxSize < 0.0f)
                {
                    contentVerticalLayout.render_vertical_spacer(-extraDescriptionBoxSize);
                }

                render_review_content(workspaceManager, selectedReview);
            }
            else if (selectedProject.m_id)
            {
                {
                    gluten::text projectDescriptionText(selectedProject.m_projectDescription);
                    projectDescriptionText.set_element_frame_padding();
                    projectDescriptionText.set_element_min_size(ImVec2(200.0f, 60.0f));
                    projectDescriptionText.set_element_content_scale(1.5f);
                    contentVerticalLayout.render_layout_element_pixels_vertical(&projectDescriptionText, 60.0f);
                    ImGui::Dummy(ImVec2(0.0f, 60.0f));
                }

                if (workspaceManager->get_user_privileges() == user_privileges::admin)
                {
                    ImGui::Dummy(ImVec2(8.0f, 0.0f));
                    ImGui::SameLine();
                    m_editReviewers.render_reviewers();

                    ImGui::Dummy(ImVec2(0.0f, 8.0f));
                    ImGui::Dummy(ImVec2(8.0f, 0.0f));
                    ImGui::SameLine();

                    if (ImGui::Button("Save"))
                    {
                        workspaceManager->set_project_users(selectedProject.m_id, m_editReviewers.get_edited_users());
                    }
                    ImGui::SetItemTooltip("Save the user list to the project");
                }
            }
            else if (!selectedUser.m_email.empty())
            {
                constexpr float avatarSize  = 300.0f;
                constexpr float paddingSize = 10.0f;
                constexpr float textSize    = 20.0f;
                constexpr float buttonSize  = textSize * 2.0f;

                user_avatar_element userAvatar(selectedUser.m_email);
                gluten::text userDisplayNameText(selectedUser.m_displayName, ImVec2(),
                                                 gluten::anchor_preset::stretch_full);
                gluten::text userTitleText(selectedUser.m_title, ImVec2(), gluten::anchor_preset::stretch_full);
                gluten::text userEmailText(selectedUser.m_email, ImVec2(), gluten::anchor_preset::stretch_full);
                gluten::text userRoleText(get_user_privileges_string(selectedUser.m_privileges), ImVec2(),
                                          gluten::anchor_preset::stretch_full);

                gluten::anchor_info& layoutAnchor = leftUserPanel.get_element_anchor();
                layoutAnchor.maxOffset.x          = avatarSize;
                leftUserPanel.set_element_background_color(gluten::theme::carbon_g100::layer02);
                leftUserPanel.set_layout_spacing(paddingSize);

                gluten::anchor_info& anchor = userAvatar.get_element_anchor();
                anchor.min = anchor.max = ImVec2(0, 0);
                anchor.maxOffset        = ImVec2(avatarSize, avatarSize);
                userAvatar.set_element_padding(ImVec2(paddingSize, paddingSize));
                userAvatar.set_avatar_render(gluten::image_render::square);

                userDisplayNameText.set_font(gluten::fonts::title).set_element_frame_padding();
                userDisplayNameText.set_element_padding(ImVec2(paddingSize, 0.0f));
                userTitleText.set_element_padding(ImVec2(paddingSize, 0.0f));
                userEmailText.set_element_padding(ImVec2(paddingSize, 0.0f));
                userRoleText.set_element_padding(ImVec2(paddingSize, 0.0f));
                editUserButton.set_element_translation(ImVec2(paddingSize, 0.0f));
                changePasswordButton.set_element_translation(ImVec2(paddingSize, 0.0f));
                deleteUserButton.set_element_translation(ImVec2(paddingSize, 0.0f));

                leftUserPanel.render(mainContentParent.get_element_rect());
                leftUserPanel.render_layout_element_pixels_vertical(&userAvatar, avatarSize);
                leftUserPanel.render_layout_element_pixels_vertical(&userDisplayNameText, textSize);
                leftUserPanel.render_layout_element_pixels_vertical(&userTitleText, textSize);
                leftUserPanel.render_layout_element_pixels_vertical(&userEmailText, textSize);
                leftUserPanel.render_layout_element_pixels_vertical(&userRoleText, textSize);

                ImGui::BeginDisabled(m_userSettings->m_loggedInUser.m_email != selectedUser.m_email &&
                                     m_userSettings->m_loggedInUser.m_privileges != user_privileges::admin);
                leftUserPanel.render_layout_element_pixels_vertical(&editUserButton, buttonSize);

                if (m_userSettings->m_loggedInUser.m_email == selectedUser.m_email ||
                    m_userSettings->m_loggedInUser.m_privileges == user_privileges::admin)
                {
                    leftUserPanel.render_layout_element_pixels_vertical(&changePasswordButton, buttonSize);

                    gluten::imgui::scoped_color_stack deleteButtonColors(ImGuiCol_Button, gluten::theme::red60,
                                                                         ImGuiCol_ButtonHovered, gluten::theme::red50,
                                                                         ImGuiCol_ButtonActive, gluten::theme::red70);
                    if (leftUserPanel.render_layout_element_pixels_vertical(&deleteUserButton, buttonSize))
                    {
                        static std::shared_ptr<gluten::confirmation_popup> confirmUserDeletionPopup;
                        confirmUserDeletionPopup = add_child_widget<gluten::confirmation_popup>(
                            false, "Delete User?",
                            [weakWorkspaceManager = m_workspaceManager, userId = selectedUser.m_userId]()
                            {
                                if (std::shared_ptr<workspace_manager> workspaceManager = weakWorkspaceManager.lock())
                                {
                                    workspaceManager->delete_user(userId);
                                }
                            });
                        confirmUserDeletionPopup->open_popup();
                    }
                }

                ImGui::EndDisabled();
            }
        }
        ImGui::EndChild();
    }
    ImGui::EndChild();
}

auto workspace_widget::render_right_panel(std::shared_ptr<workspace_manager>& workspaceManager,
                                          const review_data& selectedReview,
                                          const user_data& selectedUser) -> void
{
    rightPanelLayout.render_window();
    render_reviewers(workspaceManager, selectedReview, selectedUser);
    // render_tests();
}

void workspace_widget::render_review_content(std::shared_ptr<workspace_manager>& workspaceManager,
                                             const review_data& selectedReview)
{
    contentVerticalLayout.render_layout_element_remaining(&reviewContent);

    if (ImGui::BeginTabBar("Tabs"))
    {
        const ImGuiTabItemFlags filesTabFlags = m_focussedComment.has_value() ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;

        if (ImGui::BeginTabItem("Files", nullptr, filesTabFlags))
        {
            ImGui::Dummy(ImVec2(0.0f, ImGui::GetStyle().FramePadding.y));

            static std::unordered_map<int64_t, std::size_t> m_reviewToSelectedVersionMap;

            std::size_t maxVersions = 1;

            for (const auto& assetRef : { std::cref(selectedReview.m_relativeContextFiles), std::cref(selectedReview.m_reviewAssets) })
            {
                for (const auto& asset : assetRef.get())
                {
                    maxVersions = std::max<std::size_t>(asset.m_versionsToRelativeFiles.size(), maxVersions);
                }
            }

            if (!m_reviewToSelectedVersionMap.contains(selectedReview.m_reviewId))
            {
                m_reviewToSelectedVersionMap[selectedReview.m_reviewId] = maxVersions;
            }

            {
                gluten::imgui::scoped_color selectableBg(ImGuiCol_PopupBg, gluten::theme::carbon_g100::layer02);

                if (ImGui::BeginCombo("Review Version", fmt::format("#{}", m_reviewToSelectedVersionMap[selectedReview.m_reviewId]).c_str()))
                {
                    for (std::size_t versionIndex = 0; versionIndex < maxVersions; ++versionIndex)
                    {
                        if (ImGui::Selectable(fmt::format("#{}", versionIndex + 1).c_str()))
                        {
                            m_reviewToSelectedVersionMap[selectedReview.m_reviewId] = versionIndex + 1;
                        }
                    }

                    ImGui::EndCombo();
                }
            }

            ImGui::SameLine(0.0f, 10.0f);

            bool collapseAll = false;
            bool expandAll   = false;

            if (ImGui::Button(ICON_LC_PLUS " Version"))
            {
                createReviewPopup = add_child_widget<create_review_popup>(this, selectedReview.m_reviewId);
                createReviewPopup->onCompleteDelegate.AddLambda([reviewId = selectedReview.m_reviewId, selectedReview = std::cref(selectedReview), selectedVersionsMap = std::ref(m_reviewToSelectedVersionMap)]() 
                    {
                        std::size_t maxVersions = 1;

                        for (const auto& assetRef : { std::cref(selectedReview.get().m_relativeContextFiles), std::cref(selectedReview.get().m_reviewAssets) })
                        {
                            for (const auto& asset : assetRef.get())
                            {
                                maxVersions = std::max<std::size_t>(asset.m_versionsToRelativeFiles.size(), maxVersions);
                            }
                        }

                        selectedVersionsMap.get()[reviewId] = maxVersions;
                    });
                createReviewPopup->open_popup();
            }

            ImGui::SameLine(0.0f, 10.0f);

            if (ImGui::Button("Collapse All"))
            {
                collapseAll = true;
            }

            ImGui::SameLine(0.0f, 10.0f);

            if (ImGui::Button("Expand All"))
            {
                expandAll = true;
            }

            ImGui::Dummy(ImVec2(0.0f, ImGui::GetStyle().FramePadding.y));

            const ImVec2 currentCursorPos = ImGui::GetCursorScreenPos();
            const float width             = reviewContent.get_element_rect().GetSize().x - 25.0f;
            ImRect reviewFilesRect(currentCursorPos, ImVec2(currentCursorPos.x + width, currentCursorPos.y));
            m_reviewFilesLayout.render(reviewFilesRect);

            m_reviewFilesLayout.set_layout_spacing(0.0f);

            auto render_assets = [&](const std::vector<versionable_review_asset>& assets) 
                {
                    for (const auto& asset : assets)
                    {
                        if (asset.m_fileName.empty())
                        {
                            continue;
                        }

                        const std::size_t selectedVersion = std::min(m_reviewToSelectedVersionMap[selectedReview.m_reviewId], asset.m_versionsToRelativeFiles.size());

                        if (asset.m_versionsToRelativeFiles.contains(selectedVersion))
                        {
                            gluten::imgui::scoped_id reviewId(selectedReview.m_reviewId);
                            gluten::collapsing_header header(fmt::format("{} #{}", asset.m_fileName, selectedVersion), false);

                            if (collapseAll)
                            {
                                header.set_open(false);
                            }

                            if (expandAll)
                            {
                                header.set_open(true);
                            }

                            if (m_focussedComment.has_value() && m_focussedComment.value().fileId == asset.m_fileId)
                            {
                                header.set_open(true);
                            }

                            if (m_reviewFilesLayout.render_layout_element_percent_horizontal(&header, 1.0f))
                            {
                                const auto reviewFileCache = workspaceManager->get_review_file(asset.m_versionsToRelativeFiles.at(selectedVersion));
                                
                                if (!reviewFileCache.has_data())
                                {
                                    gluten::loading_spinner loadingFileSpinner;
                                    m_reviewFilesLayout.render_layout_element_pixels_horizontal(&loadingFileSpinner, 50.0f);
                                    continue;
                                }

                                if (audio_element::can_handle_file(reviewFileCache.m_cache))
                                {
                                    audio_element audioElement(reviewFileCache.m_cache, asset.m_fileId);
                                    if (m_reviewFilesLayout.render_layout_element_percent_horizontal(&audioElement, 1.0f))
                                    {
                                        m_createCommentPopup = add_child_widget<create_comment_popup>(this, selectedReview.m_reviewId, asset.m_fileId, audioElement.get_file_position());
                                        m_createCommentPopup->open_popup();
                                    }

                                    if (m_focussedComment.has_value())
                                    {
                                        audioElement.seek_to_position(m_focussedComment.value().filePosition);
                                        m_focussedComment.reset();
                                    }
                                }
                                else if (video_element::can_handle_file(reviewFileCache.m_cache))
                                {
                                    video_element videoElement(reviewFileCache.m_cache, asset.m_fileId);
                                    if (m_reviewFilesLayout.render_layout_element_percent_horizontal(&videoElement, 1.0f))
                                    {
                                        m_createCommentPopup = add_child_widget<create_comment_popup>(this, selectedReview.m_reviewId, asset.m_fileId, videoElement.get_file_position());
                                        m_createCommentPopup->open_popup();
                                    }

                                    if (m_focussedComment.has_value())
                                    {
                                        videoElement.seek_to_position(m_focussedComment.value().filePosition);
                                        m_focussedComment.reset();
                                    }
                                }
                            }
                        }
                    }
                };

            gluten::text contextHeader("Context Files");
            gluten::text assetsHeader("Review Files");

            contextHeader.set_font(gluten::fonts::title);
            assetsHeader.set_font(gluten::fonts::title);

            m_reviewFilesLayout.render_layout_element_percent_horizontal(&contextHeader, 1.0f);
            m_reviewFilesLayout.render_vertical_spacer(16.0f);
            render_assets(selectedReview.m_relativeContextFiles);
            m_reviewFilesLayout.render_vertical_spacer(16.0f);

            m_reviewFilesLayout.render_layout_element_percent_horizontal(&assetsHeader, 1.0f);
            m_reviewFilesLayout.render_vertical_spacer(16.0f);
            render_assets(selectedReview.m_reviewAssets);
            m_reviewFilesLayout.render_vertical_spacer(16.0f);


            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Comments"))
        {
            ImGui::Dummy(ImVec2(0.0f, ImGui::GetStyle().FramePadding.y));

            const auto& comments = workspaceManager->get_all_comments_for_review(selectedReview.m_reviewId);
            const auto& users = workspaceManager->get_all_users();
            if (comments.has_data())
            {
                for (auto iter = comments.m_cache.rbegin(); iter != comments.m_cache.rend(); ++iter)
                {
                    const auto& comment = *iter;

                    const user_data commentUser = workspaceManager->get_user(comment.m_userId);

                    constexpr float commentAvatarSize = 40.0f;
                    constexpr float commentPadding    = 8.0f;

                    gluten::imgui::scoped_id id(comment.m_commentId);
                    
                    user_avatar_element avatar(commentUser.m_email);
                    avatar.set_element_min_size(ImVec2(commentAvatarSize, commentAvatarSize));
                    avatar.render_cursor();

                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + commentAvatarSize + commentPadding);
                    ImGui::BeginGroup();
                    ImGui::TextUnformatted(fmt::format("{} commented on review {}", commentUser.m_displayName, selectedReview.m_reviewId).c_str());
                    if (comment.m_fileId)
                    {
                        std::string fileName;

                        auto foundFileIter = std::find_if(
                            selectedReview.m_relativeContextFiles.begin(),
                            selectedReview.m_relativeContextFiles.end(),
                            [fileId = comment.m_fileId](const versionable_review_asset& asset) {return asset.m_fileId == fileId;});

                        if (foundFileIter == selectedReview.m_relativeContextFiles.end())
                        {
                            foundFileIter = std::find_if(
                                selectedReview.m_reviewAssets.begin(),
                                selectedReview.m_reviewAssets.end(),
                                [fileId = comment.m_fileId](const versionable_review_asset& asset){return asset.m_fileId == fileId;});

                            if (foundFileIter != selectedReview.m_reviewAssets.end())
                            {
                                fileName = foundFileIter->m_fileName;
                            }
                        }
                        else
                        {
                            fileName = foundFileIter->m_fileName;
                        }

                        ImGui::SameLine();
                        if (comment.m_timeStart)
                        {
                            ImGui::TextUnformatted(fmt::format(" - {} at {:.2f}", fileName, comment.m_timeStart).c_str());
                        }
                        else
                        {
                            ImGui::TextUnformatted(fmt::format(" - {}", fileName).c_str());
                        }
                    }
                    ImGui::Dummy(ImVec2(0.0f, commentPadding * 2.0f));
                    ImGui::TextWrapped(comment.m_comment.c_str());
                    ImGui::Dummy(ImVec2(0.0f, commentPadding * 2.0f));
                    if (comment.m_fileId > 0)
                    {
                        if (ImGui::Button(ICON_LC_SQUARE_ARROW_OUT_UP_RIGHT))
                        {
                            m_focussedComment = focussed_comment {.commentId = comment.m_commentId, .fileId = comment.m_fileId, .filePosition = comment.m_timeStart};
                        }
                        ImGui::SetItemTooltip("Go to file comment...");
                    }
                    ImGui::EndGroup();

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
            }
            else
            {
                gluten::loading_spinner loading;
                loading.render_cursor();
            }

            static bool newCommentOpen = false;

            ImGui::BeginDisabled(newCommentOpen);
            if (m_userSettings->m_loggedInUser.m_privileges > user_privileges::guest)
            {
                if (ImGui::Button(ICON_LC_PLUS))
                {
                    newCommentOpen = true;
                }
            }
            ImGui::EndDisabled();

            constexpr std::size_t commentBufferSize = 2045;
            static char buffer[commentBufferSize]   = {0};

            if (newCommentOpen)
            {
                ImGui::Dummy(ImVec2(0.0f, ImGui::GetStyle().FramePadding.y));

                bool wantsToAdd = false;

                if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Enter))
                {
                    wantsToAdd = true;
                }

                ImGui::InputTextMultiline("Comment", buffer, commentBufferSize,
                                            ImVec2(ImGui::GetWindowSize().x, 100.0f));

                ImGui::Dummy(ImVec2(0.0f, ImGui::GetStyle().FramePadding.y));

                wantsToAdd |= ImGui::Button("Add");

                if (wantsToAdd)
                {
                    new_comment_data newComment;
                    newComment.m_reviewId       = selectedReview.m_reviewId;
                    newComment.m_comment        = buffer;

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

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Activity"))
        {
            const auto& reviewsActivity =
                workspaceManager->get_all_review_activity(selectedReview.m_reviewId);
            if (reviewsActivity.has_data())
            {
                for (const auto& iter : reviewsActivity.m_cache)
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
                        activityTimeText =
                            std::to_string(duration_cast<std::chrono::minutes>(diff).count()) + " minutes ago";
                    }
                    else if (diff < 24h)
                    {
                        activityTimeText =
                            std::to_string(duration_cast<std::chrono::hours>(diff).count()) + " hours ago";
                    }
                    else
                    {
                        auto days = duration_cast<std::chrono::duration<int, std::ratio<86400>>>(diff).count();
                        activityTimeText = std::to_string(days) + " day" + (days > 1 ? "s" : "") + " ago";
                    }

                    ImGui::Dummy(ImVec2(0, ImGui::GetStyle().FramePadding.y));
                    ImGui::Text(fmt::format("{} {}", iter.m_activityText, activityTimeText).c_str());
                }
            }
            else
            {
                gluten::loading_spinner loading;
                loading.render_cursor();
            }

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}

void workspace_widget::render_review_description(const review_data& selectedReview)
{
    constexpr float descriptionBoxStartingHeight = 70.0;

    contentVerticalLayout.render_layout_element_pixels_vertical(&descriptionBoxLayout, descriptionBoxStartingHeight);

    titleText.set_text(fmt::format("{}", selectedReview.m_reviewName));
    titleText.set_url(selectedReview.m_reviewTaskUrl);
    descriptionText.set_text(fmt::format("{} {}", ICON_LC_MESSAGE_CIRCLE, selectedReview.m_reviewDescription));
    phaseText.set_text(fmt::format("{} {}", ICON_LC_WORKFLOW, get_review_phase_string(selectedReview.m_reviewPhase)));
    qualityText.set_text(fmt::format("{} {}", ICON_LC_AWARD, get_review_quality_string(selectedReview.m_reviewQuality)));

    std::pair<std::string, int> votes = get_votes_string(selectedReview);
    votesText.set_text(fmt::format("     {}", votes.first));

    descriptionBoxLayout.render_layout_element_pixels_vertical(&titleText, 40.0f);
    descriptionBoxLayout.render_vertical_spacer(10.0f);

    const float descriptionTextHeight = ImGui::CalcTextSize(selectedReview.m_reviewDescription.c_str(), nullptr, descriptionBoxLayout.get_element_rect().GetWidth()).y * 1.4f;
    descriptionBoxLayout.render_layout_element_pixels_vertical(&descriptionText, descriptionTextHeight);
    descriptionBoxLayout.render_vertical_spacer(16.0f);
    descriptionBoxLayout.render_layout_element_pixels_vertical(&votesIconText, 30.0f);
    
    {
        ImVec4 votesColor;

        if (votes.second > 0)
        {
            votesColor = gluten::theme::carbon_g100::supportSuccess;
        }
        else if (votes.second == 0)
        {
            votesColor = gluten::theme::carbon_g100::supportCautionMinor;
        }
        else
        {
            votesColor = gluten::theme::carbon_g100::supportError;
        }

        gluten::imgui::scoped_color votesTextColor(ImGuiCol_Text, votesColor);
        votesText.render(votesIconText.get_element_rect());
    }

    gluten::imgui::scoped_color frameProgressBg(ImGuiCol_FrameBg, gluten::theme::carbon_g100::layer02);

    constexpr float max       = ((int)review_phase::num) * ((int)review_quality::num);
    constexpr float third     = max * 0.33f;
    constexpr float twoThirds = max * 0.66f;
    const float value         = ((int)selectedReview.m_reviewPhase + 1) * ((int)selectedReview.m_reviewQuality + 1);
    const float fraction      = value / max;

    gluten::imgui::scoped_color progressBarColor(ImGuiCol_PlotHistogram, value >= twoThirds ? gluten::theme::red60
                                                                         : value >= third   ? gluten::theme::yellow60
                                                                                            : gluten::theme::green60);
    m_descriptionBoxButtonsLayout.render(descriptionBoxLayout.get_element_rect());

    if (std::shared_ptr<workspace_manager> workspaceManager = m_workspaceManager.lock())
    {
        if (selectedReview.m_reviewStatus == review_status::open && m_userSettings->m_loggedInUser.m_privileges > user_privileges::guest)
        {
            if (m_descriptionBoxButtonsLayout.render_layout_element_pixels_horizontal(&descriptionEditButton, 30.0f))
            {
                static std::shared_ptr<update_review_popup> updateProjectPopup;
                updateProjectPopup = add_child_widget<update_review_popup>(false);

                if (updateProjectPopup)
                {
                    updateProjectPopup->set_review_data(selectedReview);
                    updateProjectPopup->open_popup();
                }
            }
        }

        const auto& reviewers = workspaceManager->get_review_users(selectedReview.m_reviewId);
        if (selectedReview.m_reviewStatus == review_status::open && reviewers.has_data())
        {
            const auto foundIter = std::find_if(reviewers.m_cache.begin(), reviewers.m_cache.end(),
                         [loggedInUserId = m_userSettings->m_loggedInUser.m_userId](const reviewer_data& reviewer) 
                {
                    return reviewer.m_userId == loggedInUserId;
                });

            if (foundIter != reviewers.m_cache.end())
            {
                gluten::button downVoteButton(ICON_LC_THUMBS_DOWN);
                gluten::button noVoteButton(ICON_LC_MINUS);
                gluten::button upVoteButton(ICON_LC_THUMBS_UP);

                if (m_descriptionBoxButtonsLayout.render_layout_element_pixels_horizontal(
                        &upVoteButton, 30.0f))
                {
                    workspaceManager->set_review_vote(selectedReview.m_reviewId, m_userSettings->m_loggedInUser.m_userId, review_vote::upvote);
                }

                if (m_descriptionBoxButtonsLayout.render_layout_element_pixels_horizontal(
                        &noVoteButton, 30.0f))
                {
                    workspaceManager->set_review_vote(selectedReview.m_reviewId, m_userSettings->m_loggedInUser.m_userId, review_vote::no_vote);
                }

                if (m_descriptionBoxButtonsLayout.render_layout_element_pixels_horizontal(
                        &downVoteButton, 30.0f))
                {
                    workspaceManager->set_review_vote(selectedReview.m_reviewId, m_userSettings->m_loggedInUser.m_userId, review_vote::downvote);
                }
            }
        }

        m_descriptionBoxButtonsLayout.render_layout_element_pixels_horizontal(nullptr, ImGui::CalcItemWidth() + ImGui::CalcTextSize("Status", NULL, true).x);

        ImGui::SetCursorScreenPos(m_descriptionBoxButtonsLayout.get_current_layout_pos());

        if (ImGui::BeginCombo("Status", selectedReview.m_reviewStatus == review_status::open ? "Open" : selectedReview.m_reviewStatus == review_status::closed ? "Closed" : "Archived"))
        {
            bool reviewOpen = selectedReview.m_reviewStatus == review_status::open;
            bool reviewClosed = selectedReview.m_reviewStatus == review_status::closed;
            bool reviewArchived = !reviewOpen && !reviewClosed;

            if (ImGui::Selectable("Open", &reviewOpen))
            {
                workspaceManager->set_review_status(selectedReview.m_reviewId, review_status::open);
            }

            if (ImGui::Selectable("Closed", &reviewClosed))
            {
                workspaceManager->set_review_status(selectedReview.m_reviewId, review_status::closed);
            }

            if (ImGui::Selectable("Archived", &reviewArchived))
            {
                workspaceManager->set_review_status(selectedReview.m_reviewId, review_status::archived);
            }

            ImGui::EndCombo();
        }
    }
}

void workspace_widget::render_tests()
{
    /*if (rightPanelBackground.render_layout_element_pixels_vertical(&testsHeader, 50.0f))
    {
    }*/
}

void workspace_widget::render_reviewers(std::shared_ptr<workspace_manager>& workspaceManager,
                                        const review_data& selectedReview,
                                        const user_data& selectedUser)
{
    const auto& reviewers = workspaceManager->get_review_users(selectedReview.m_reviewId);

    rightPanelLayout.render_layout_element_pixels_vertical(nullptr, 2.0f);
        
    if (reviewers.has_data())
    {
        for (const auto& reviewer : reviewers.m_cache)
        {
            reviewer_display_element user(reviewer, selectedReview.m_reviewId);
            rightPanelLayout.render_layout_element_pixels_vertical(&user, rightPanelLayout.get_element_rect().GetWidth());
        }
    }
    else if (reviewers.m_state == gluten::cache_state::loading)
    {
        gluten::loading_spinner loading;
        rightPanelLayout.render_layout_element_percent_vertical(&loading, 1.0f);
    }

    /*if (m_userSettings->m_loggedInUser.m_privileges > user_privileges::guest)
    {
        if (editReviewersButton.render(reviewersHeader.get_element_rect()))
        {
            static std::shared_ptr<edit_reviewers_popup> editReviewersPopup;
            editReviewersPopup = add_child_widget<edit_reviewers_popup>(false, selectedReview);
            editReviewersPopup->open_popup();
        }
    }*/
}

void workspace_widget::render_top_content_bar(std::shared_ptr<workspace_manager>& workspaceManager,
                                              const project_data& selectedProject,
                                              const review_data& selectedReview)
{
    topContentBarBackground.render_window();

    if (ImDrawList* const drawList = ImGui::GetWindowDrawList())
    {
        //drawList->AddLine(topContentBarBackground.get_element_rect().GetTL(), topContentBarBackground.get_element_rect().GetBL(), ImGui::ColorConvertFloat4ToU32(gluten::theme::carbon_g100::borderStrong01), 1.0f);
        drawList->AddLine(topContentBarBackground.get_element_rect().GetBL(), topContentBarBackground.get_element_rect().GetBR(), ImGui::ColorConvertFloat4ToU32(gluten::theme::carbon_g100::borderStrong02), 2.0f);
    }

    const auto& workspaceName = workspaceManager->get_workspace_name();

    if (workspaceName.has_data())
    {
        std::string breadcrumbString = workspaceName.m_cache;

        if (selectedProject.m_id != 0)
        {
            breadcrumbString += " / " + selectedProject.m_projectName;
        }

        if (selectedReview.m_reviewId != 0)
        {
            breadcrumbString += " / " + selectedReview.m_reviewName;
        }

        breadcrumbText.set_text(breadcrumbString);
        breadcrumbText.render(topContentBarBackground.get_element_rect());
    }
    else
    {
        gluten::loading_spinner loading;
        loading.render(topContentBarBackground.get_element_rect());
    }

    logged_in_user_element loggedInUserAvatar(m_userSettings->m_loggedInUser.m_email);
    loggedInUserAvatar.set_element_anchor_preset(gluten::anchor_preset::stretch_right);
    loggedInUserAvatar.get_element_anchor().minOffset.x = -topContentBarBackground.get_element_rect().GetHeight();
    loggedInUserAvatar.render(topContentBarBackground.get_element_rect());
}

auto workspace_widget::render_left_toolbar() -> void
{
    gluten::imgui::scoped_color toolbarBackgroundColor(ImGuiCol_ChildBg, gluten::theme::carbon_g100::layer01);

    std::shared_ptr<workspace_manager> workspaceManager = m_workspaceManager.lock();

    if (!workspaceManager)
    {
        return;
    }

    if (ImGui::BeginChild("LeftToolbar", ImVec2(leftToobarWidth, 0)))
    {
        buttonsLayout.render_window();

        static auto create_toolbar_button = [activeView = std::cref(m_activeView)](const char* name, const char* icon, active_view active) 
            {
                gluten::icon_button iconButton(name, icon, gluten::fonts::regular_lucide_icons);
                iconButton
                    .set_element_content_font_size(gluten::g_baseIconFontSize * (leftToolbarButtonHeight / gluten::g_baseIconFontSize) / 1.75f)
                    .set_element_hover_color(gluten::theme::carbon_g100::backgroundHover)
                    .set_element_active_color(gluten::theme::carbon_g100::backgroundActive)
                    .set_element_active(activeView == active);
                return iconButton;
            };

        gluten::icon_button reviewsButton = create_toolbar_button("##ReviewsButton", ICON_LC_CHART_NO_AXES_GANTT, reviews_view);
        gluten::icon_button usersButton = create_toolbar_button("##UsersButton", ICON_LC_USERS, users_view);
        gluten::icon_button settingsButton = create_toolbar_button("##SettingsButton", ICON_LC_SETTINGS, settings_view);

        if (buttonsLayout.render_layout_element_pixels_vertical(&reviewsButton, leftToolbarButtonHeight))
        {
            m_activeView = reviews_view;
            workspaceManager->select_project({});
            workspaceManager->select_user({});
        }

        if (buttonsLayout.render_layout_element_pixels_vertical(&usersButton, leftToolbarButtonHeight))
        {
            m_activeView = users_view;
            workspaceManager->select_project({});
            workspaceManager->select_user({});
        }

        if (buttonsLayout.render_layout_element_pixels_vertical(&settingsButton, leftToolbarButtonHeight))
        {
            m_activeView = settings_view;
            workspaceManager->select_project({});
            workspaceManager->select_user({});
        }
    }
    ImGui::EndChild();
}

auto workspace_widget::get_votes_string(const review_data& selectedReview) const -> std::pair<std::string, int>
{
    const auto reviewers = m_workspaceManager.lock()->get_review_users(selectedReview.m_reviewId);

    std::string text;
    int upvotes = 0;
    int downvotes = 0;

    if (reviewers.has_data())
    {
        for (const auto& reviewer : reviewers.m_cache)
        {
            if (reviewer.m_vote == review_vote::upvote)
            {
                ++upvotes;
            }

            if (reviewer.m_vote == review_vote::downvote)
            {
                ++downvotes;
            }
        }

        for (int i = 0; i < upvotes; ++i)
        {
            text += ICON_LC_THUMBS_UP;
        }

        if (upvotes > 0 && downvotes > 0)
        {
            text += " | ";
        }

        for (int i = 0; i < downvotes; ++i)
        {
            text += ICON_LC_THUMBS_DOWN;
        }
    }

    return {text, upvotes - downvotes};
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
            review_app::reset_to_intro();
        }

        if (ImGui::MenuItem("Logout..."))
        {
            get_app()->get_manager_by_class<workspace_manager>()->logout();
        }
        ImGui::EndMenu();
    }
}

auto workspace_widget::render_settings() -> void
{
    gluten::imgui::scoped_color bgColor(ImGuiCol_FrameBg, gluten::theme::carbon_g100::layer03);
    gluten::imgui::scoped_style padding(ImGuiStyleVar_WindowPadding, ImVec2(80.0f, 80.0f));

    if (ImGui::BeginChild("SettingsContainer", ImVec2(0, 0), ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoScrollbar))
    {
        gluten::background settingsBackground;
        settingsBackground.set_element_background_color(gluten::theme::carbon_g100::layer02);
        settingsBackground.render_window();

        if (ImGui::BeginTable("Settings", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingStretchProp))
        {
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Setting");
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Value");
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Theme");
            ImGui::TableNextColumn();
            
            if (ImGui::BeginCombo("Theme", m_userSettings->m_theme == review_app_theme::dark ? "Dark" : "Light"))
            {
                bool darkSelected = m_userSettings->m_theme == review_app_theme::dark;
                bool lightSelected = m_userSettings->m_theme == review_app_theme::light;

                if (ImGui::Selectable("Dark", &darkSelected))
                {
                    m_userSettings->m_theme = review_app_theme::dark;
                }

                if (ImGui::Selectable("Light", &lightSelected))
                {
                    m_userSettings->m_theme = review_app_theme::light;
                }

                ImGui::EndCombo();
            }

            ImGui::EndTable();
        }
    }
    ImGui::EndChild();
}