#pragma once

#include "pch.h"

#include "data/review_data.h"
#include "app/review_database.h"
#include "misc/edit_reviewers.h"

class workspace_manager;

class edit_reviewers_popup : public gluten::popup_widget, protected edit_reviewers
{
public:
    edit_reviewers_popup(gluten::widget* widgetParent, const review_data& review) : gluten::popup_widget(widgetParent, "Edit Reviewers"), m_review(review), edit_reviewers() 
    {
        set_review_id(m_review.m_reviewId);
    }

protected:
    auto start_implementation() -> void override;
    auto render_popup() -> void override;

private:
    review_data m_review;
    std::weak_ptr<workspace_manager> m_workspaceManager;
};