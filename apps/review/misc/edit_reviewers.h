#pragma once

#include "pch.h"

#include "data/review_data.h"

class edit_reviewers
{
public:
    edit_reviewers() = delete;
    edit_reviewers(int64_t reviewId) : m_reviewId(reviewId) {}

protected:
    auto render_reviewers() -> void;

    std::optional<std::vector<reviewer_data>> m_newReviewers;
    bool m_addingNewUser = false;

    int64_t m_reviewId = 0;
};