#pragma once

#include "pch.h"

#include "data/user_settings_data.h"
#include "data/review_data.h"
#include "data/project_data.h"

class workspace_manager;

class workspace_widget : public gluten::window_widget
{
    WIDGET_CONSTRUCT_PARENT(workspace_widget, "Workspace", gluten::window_widget)

protected:
    virtual auto start_implementation() -> void override;
    virtual auto render_window_implementation() -> void override;
    virtual auto render_menu_implementation() -> void override;

private:
    auto render_content() -> void;
    auto render_right_panel(std::shared_ptr<workspace_manager> &workspaceManager,
                            const review_data &selectedReview,
                            const user_data &selectedUser) -> void;
    void render_review_content(std::shared_ptr<workspace_manager> &workspaceManager, const review_data& selectedReview);
    void render_review_description(const review_data &selectedReview);
    void render_tests();
    void render_reviewers(std::shared_ptr<workspace_manager> &workspaceManager, const review_data &selectedReview, const user_data& selectedUser);
    void render_top_content_bar(std::shared_ptr<workspace_manager> &workspaceManager,
                                const project_data &selectedProject,
                                const review_data &selectedReview);
    auto render_list() -> void;
    auto render_left_toolbar() -> void;

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
    gluten::layout contentAndRightPanelLayout = gluten::layout(gluten::layout::layout_type::right_to_left, gluten::element::anchor_preset::stretch_full);
    gluten::layout rightPanelLayout = gluten::layout(gluten::layout::layout_type::top_to_bottom, gluten::element::anchor_preset::stretch_right);
    gluten::layout contentVerticalLayout = gluten::layout(gluten::layout::layout_type::top_to_bottom, gluten::element::anchor_preset::stretch_full);
    gluten::layout buttonsLayout = gluten::layout(gluten::layout::layout_type::top_to_bottom, gluten::element::anchor_preset::stretch_full);
    gluten::layout leftUserPanel = gluten::layout(gluten::layout_type::top_to_bottom, gluten::anchor_preset::stretch_left);
    gluten::layout descriptionBoxLayout;
    gluten::layout scrutinyLayout;
    gluten::layout m_reviewFilesLayout = gluten::layout(gluten::layout_type::top_to_bottom, gluten::anchor_preset::stretch_full);

    gluten::background topContentBarBackground;
    gluten::background mainContentParent;

    gluten::text breadcrumbText     = gluten::text({}, ImVec2(0, -0.75f), gluten::element::anchor_preset::left_top);
    gluten::text titleText          = gluten::text({}, ImVec2(0.0f, 0.0f), gluten::element::anchor_preset::stretch_full);
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

    std::shared_ptr<class create_comment_popup> m_createCommentPopup;
};
