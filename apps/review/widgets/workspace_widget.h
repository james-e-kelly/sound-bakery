#pragma once

#include "pch.h"

#include "data/user_settings_data.h"
#include "data/review_data.h"
#include "data/project_data.h"
#include "misc/edit_reviewers.h"

class workspace_manager;

struct focussed_comment
{
    int64_t commentId = 0;
    int64_t fileId    = 0;
    double filePosition = 0.0;
};

class workspace_widget : public gluten::window_widget
{
    WIDGET_CONSTRUCT_PARENT(workspace_widget, "Workspace", gluten::window_widget)

protected:
    virtual auto start_implementation() -> void override;
    virtual auto render_window_implementation() -> void override;
    virtual auto render_menu_implementation() -> void override;

private:
    auto render_left_toolbar() -> void;

    auto render_list_panel() -> void;
    auto render_list_panel_elements(const bool listingProjects, std::shared_ptr<workspace_manager> &workspaceManager, const bool listingReviews) -> void;

    auto render_content() -> void;
    auto render_right_panel(std::shared_ptr<workspace_manager> &workspaceManager, const review_data &selectedReview, const user_data &selectedUser) -> void;
    void render_review_content(std::shared_ptr<workspace_manager> &workspaceManager, const review_data& selectedReview);
    void render_review_description(const review_data &selectedReview);
    void render_tests();
    void render_reviewers(std::shared_ptr<workspace_manager> &workspaceManager, const review_data &selectedReview, const user_data& selectedUser);
    void render_top_content_bar(std::shared_ptr<workspace_manager> &workspaceManager, const project_data &selectedProject, const review_data &selectedReview);
    auto render_settings() -> void;

    auto get_votes_string(const review_data &selectedReview) const -> std::pair<std::string, int>;

    enum active_view
    {
        reviews_view,
        users_view,
        settings_view
    };

    active_view m_activeView = reviews_view;
    std::weak_ptr<workspace_manager> m_workspaceManager;
    gluten::data_source<user_settings_data> m_userSettings;

private:
    gluten::layout m_windowLayout                   = gluten::layout(gluten::layout_type::left_to_right, gluten::anchor_preset::stretch_full);  // Full window
    gluten::layout m_innerLayout                    = gluten::layout(gluten::layout_type::left_to_right, gluten::anchor_preset::stretch_full);  // Inner window (excludes the left toolbar) and has some rounding
    gluten::layout m_leftToolbarLayout              = gluten::layout(gluten::layout_type::top_to_bottom, gluten::anchor_preset::stretch_full);  // Buttons to select reviews, users, settings
    gluten::layout m_leftPanelLayout                = gluten::layout(gluten::layout_type::top_to_bottom, gluten::anchor_preset::stretch_full);  // List of reviews, users, etc.
    gluten::layout m_mainPanelLayout                = gluten::layout(gluten::layout_type::top_to_bottom, gluten::anchor_preset::stretch_full);  // The main content of header, content, and right panel of users

    gluten::layout m_contentAndRightPanelLayout     = gluten::layout(gluten::layout::layout_type::right_to_left, gluten::element::anchor_preset::stretch_full);
    gluten::layout m_rightPanelLayout               = gluten::layout(gluten::layout::layout_type::top_to_bottom, gluten::element::anchor_preset::stretch_full);
    gluten::layout m_contentVerticalLayout          = gluten::layout(gluten::layout::layout_type::top_to_bottom, gluten::element::anchor_preset::stretch_top);
    gluten::layout m_leftUserPanel                  = gluten::layout(gluten::layout_type::top_to_bottom, gluten::anchor_preset::stretch_left);
    gluten::layout m_descriptionBoxLayout;
    gluten::layout m_scrutinyLayout;
    gluten::layout m_reviewFilesLayout              = gluten::layout(gluten::layout_type::top_to_bottom, gluten::anchor_preset::stretch_full);
    gluten::layout m_descriptionBoxButtonsLayout    = gluten::layout(gluten::layout_type::right_to_left, gluten::anchor_preset::right_top);

    gluten::background m_topContentBarBackground;
    gluten::background mainContentParent;

    gluten::text listItemsTitle     = gluten::text({}, ImVec2(0.5f, 0.5f), gluten::element::anchor_preset::center_middle);
    gluten::text m_breadcrumbText   = gluten::text({}, ImVec2(0, -0.75f), gluten::element::anchor_preset::left_top);
    gluten::text m_contentTitleText = gluten::text({}, ImVec2(0.0f, 0.1f), gluten::element::anchor_preset::stretch_full);
    gluten::text descriptionText    = gluten::text({}, ImVec2(0.0f, 0.0f), gluten::element::anchor_preset::stretch_full);
    gluten::text phaseText          = gluten::text({}, ImVec2(0.0f, 0.0f), gluten::element::anchor_preset::stretch_full);
    gluten::text qualityText        = gluten::text({}, ImVec2(0.0f, 0.0f), gluten::element::anchor_preset::stretch_full);
    gluten::text votesText          = gluten::text({}, ImVec2(0.0f, 0.0f), gluten::element::anchor_preset::stretch_full);
    gluten::text votesIconText      = gluten::text(ICON_LC_VOTE, ImVec2(0.0f, 0.0f), gluten::element::anchor_preset::stretch_full);

    gluten::collapsing_header reviewersHeader = gluten::collapsing_header("Reviewers");
    gluten::collapsing_header testsHeader = gluten::collapsing_header("Tests");

    gluten::background reviewContent;

    gluten::button editReviewersButton = gluten::button("Edit " ICON_LC_PENCIL, false, gluten::anchor_preset::right_top);
    gluten::button descriptionEditButton = gluten::button("Edit " ICON_LC_PENCIL_LINE, false, gluten::element::anchor_preset::right_top);
    gluten::button editUserButton = gluten::button("Edit " ICON_LC_PENCIL);
    gluten::button changePasswordButton = gluten::button("Change Password " ICON_LC_PENCIL);
    gluten::button deleteUserButton = gluten::button("Delete " ICON_LC_USER_ROUND_X);

    gluten::icon_button m_leftPanelBackButton   = gluten::icon_button("##BackButton", ICON_LC_ARROW_BIG_LEFT, gluten::fonts::regular_lucide_icons);
    gluten::icon_button m_leftPanelNewButton    = gluten::icon_button("##NewButton", ICON_LC_PLUS, gluten::fonts::regular_lucide_icons);

    std::shared_ptr<class create_comment_popup> m_createCommentPopup;
    std::shared_ptr<class create_review_popup> createReviewPopup;

    std::optional<focussed_comment> m_focussedComment;

    edit_reviewers m_editReviewers;

    gluten::data_cache<std::filesystem::path, gluten::key_and_token_cache_key<std::filesystem::path, std::string>, gluten::key_and_token_cache_key_hasher<std::filesystem::path, std::string>> m_filesCache;
};
