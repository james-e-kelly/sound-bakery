#pragma once

#include "pch.h"

#include "data/review_data.h"

class edit_reviewers
{
public:
    edit_reviewers() = default;

    auto set_review_id(int64_t reviewId) -> void 
    { 
        m_reviewId = reviewId; 
        m_newUsers.reset();
    }

    auto set_project_id(int64_t projectId) -> void 
    { 
        m_projectId = projectId; 
        m_newUsers.reset();
    }

    auto render_reviewers() -> void;

    auto get_edited_users() const -> std::vector<user_data>
    {
        return m_newUsers.value();
    }

protected:
    std::optional<std::vector<user_data>> m_newUsers;
    bool m_addingNewUser = false;

    int64_t m_reviewId = 0;
    int64_t m_projectId = 0;
};