#pragma once

#include "pch.h"

#include "data/review_data.h"
#include "app/review_database.h"

class workspace_manager;

class edit_reviewers_popup : public gluten::popup_widget
{
public:
    edit_reviewers_popup(gluten::widget* widgetParent, const review_data& review) : gluten::popup_widget(widgetParent, "Edit Reviewers"), m_review(review) {}

protected:
    auto start_implementation() -> void override;
    auto render_popup() -> void override;

private:
    review_data m_review;
    std::weak_ptr<workspace_manager> m_workspaceManager;

    std::vector<user_data> m_allUsers; 
    std::vector<reviewer_data> m_newUsers;  

    bool m_addingNewUser = false;
};