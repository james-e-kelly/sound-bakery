#pragma once

#include "pch.h"

#include "data/project_data.h"
#include "data/review_data.h"
#include "data/user_settings_data.h"
#include "elements/left_panel_element.h"
#include "elements/toolbar_element.h"
#include "misc/edit_reviewers.h"

class workspace_manager;
class settings_popup;

struct focussed_comment
{
    int64_t commentId   = 0;
    int64_t fileId      = 0;
    double filePosition = 0.0;
};

class workspace_widget : public gluten::window_widget
{
    WIDGET_CONSTRUCT_PARENT(workspace_widget, "Workspace", gluten::window_widget)

protected:
    auto start_implementation() -> void override;
    auto refresh_style_implementation() -> void override;
    auto render_window_implementation() -> void override;
    auto render_menu_implementation() -> void override;

private:
    auto render_left_toolbar() -> void;

    auto render_content() -> void;
    auto render_right_panel(std::shared_ptr<workspace_manager> &workspaceManager, const review_data &selectedReview, const user_data &selectedUser) -> void;
    void render_review_content(std::shared_ptr<workspace_manager> &workspaceManager, const review_data &selectedReview);
    void render_review_description(const review_data &selectedReview);
    void render_tests();
    void render_reviewers(std::shared_ptr<workspace_manager> &workspaceManager, const review_data &selectedReview, const user_data &selectedUser);
    void render_top_content_bar(std::shared_ptr<workspace_manager> &workspaceManager, const project_data &selectedProject, const review_data &selectedReview);

    auto get_votes_string(const review_data &selectedReview) const -> std::pair<std::string, int>;

    std::weak_ptr<workspace_manager> m_workspaceManager;
    gluten::data_source<user_settings_data> m_userSettings;

private:
    
    // Main layouts

    gluten::layout m_windowLayout      = gluten::layout(gluten::layout_type::left_to_right, gluten::anchor_preset::stretch_full);   // Full window
    gluten::layout m_innerWindowLayout = gluten::layout(gluten::layout_type::left_to_right, gluten::anchor_preset::stretch_full);   // Window, minus the left toolbar. Let's us have the rounded edge around the workspace

    // Left toolbar

    toolbar_element m_toolbar;
    
    // Left panel (project list, review list, user list)

    left_panel_element m_leftPanel;

    // Main panel (header bar, content)

    gluten::layout m_mainPanelLayout             = gluten::layout(gluten::layout_type::top_to_bottom, gluten::anchor_preset::stretch_full);
    gluten::background m_topContentBarBackground = gluten::background(gluten::anchor_preset::stretch_full);  // Top bar. Breadcrumbs, logged in user

    gluten::layout m_contentAndRightPanelLayout = gluten::layout(gluten::layout::layout_type::right_to_left, gluten::element::anchor_preset::stretch_full);
    gluten::layout m_rightPanelLayout           = gluten::layout(gluten::layout::layout_type::top_to_bottom, gluten::element::anchor_preset::stretch_full);
    gluten::layout m_contentVerticalLayout      = gluten::layout(gluten::layout::layout_type::top_to_bottom, gluten::element::anchor_preset::stretch_top);
    gluten::layout m_leftUserPanel              = gluten::layout(gluten::layout_type::top_to_bottom, gluten::anchor_preset::stretch_left);
    gluten::layout m_descriptionBoxLayout;
    gluten::layout m_reviewFilesLayout           = gluten::layout(gluten::layout_type::top_to_bottom, gluten::anchor_preset::stretch_full);
    gluten::layout m_descriptionBoxButtonsLayout = gluten::layout(gluten::layout_type::right_to_left, gluten::anchor_preset::right_top);

    gluten::background mainContentParent;

    gluten::text m_contentTitleText       = gluten::text({}, ImVec2(0.0f, 0.0f), gluten::element::anchor_preset::stretch_full, gluten::text_style::h1, gluten::fonts::title_lucide_icons);
    gluten::text m_contentDescriptionText = gluten::text({}, ImVec2(0.0f, 0.0f), gluten::element::anchor_preset::stretch_full, gluten::text_style::subtitle, gluten::fonts::regular_lucide_icons);
    gluten::text phaseText                = gluten::text({}, ImVec2(0.0f, 0.0f), gluten::element::anchor_preset::stretch_full, gluten::text_style::helper, gluten::fonts::regular_lucide_icons);
    gluten::text qualityText              = gluten::text({}, ImVec2(0.0f, 0.0f), gluten::element::anchor_preset::stretch_full, gluten::text_style::helper, gluten::fonts::regular_lucide_icons);
    gluten::text votesText                = gluten::text({}, ImVec2(0.0f, 0.0f), gluten::element::anchor_preset::stretch_full, gluten::text_style::helper, gluten::fonts::regular_lucide_icons);
    gluten::text votesIconText            = gluten::text(ICON_LC_VOTE, ImVec2(0.0f, 0.0f), gluten::element::anchor_preset::stretch_full, gluten::text_style::helper, gluten::fonts::regular_lucide_icons);

    gluten::background reviewContent;

    gluten::button editReviewersButton   = gluten::button("Edit " ICON_LC_PENCIL,               false, gluten::anchor_preset::right_top);
    gluten::button descriptionEditButton = gluten::button(ICON_LC_PENCIL_LINE " Edit",          false, gluten::element::anchor_preset::right_top);
    gluten::button editUserButton        = gluten::button("Edit " ICON_LC_PENCIL,               false, gluten::anchor_preset::left_top);
    gluten::button changePasswordButton  = gluten::button("Change Password " ICON_LC_PENCIL,    false, gluten::anchor_preset::left_top);
    gluten::button deleteUserButton      = gluten::button("Delete " ICON_LC_USER_ROUND_X,       false, gluten::anchor_preset::left_top, gluten::button_style::danger);

    std::shared_ptr<class create_comment_popup> m_createCommentPopup;
    std::shared_ptr<class create_review_popup> createReviewPopup;

    std::optional<focussed_comment> m_focussedComment;

    edit_reviewers m_editReviewers;

    gluten::data_cache<std::filesystem::path, gluten::key_and_token_cache_key<std::filesystem::path, std::string>, gluten::key_and_token_cache_key_hasher<std::filesystem::path, std::string>> m_filesCache;
};
