#pragma once

#include "pch.h"

enum class activity_type
{
    project_created,
    project_edited,
    review_created,
    review_edited,
    review_files_edited,
    comment_added,
    comment_edited
};

auto get_activity_type_string(activity_type activityType) -> std::string;

struct activity_data
{
    int64_t m_activityId = 0;
    int64_t m_reviewId = 0;
    int64_t m_projectId = 0;
    int64_t m_userId = 0;
    activity_type m_activityType;
    std::string m_activityText;
    std::string m_activityTimestamp;
};