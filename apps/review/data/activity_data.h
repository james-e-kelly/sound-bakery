#pragma once

#include "pch.h"

enum class activity_type
{
    project_created,
    project_edited,
    project_deleted,
    review_created,
    review_edited,
    review_deleted,
    review_file_deleted,
    review_files_edited,
    comment_added,
    comment_edited,
    comment_deleted,
    user_added,
    user_edited,
    user_deleted
};

auto get_activity_type_string(activity_type activityType) -> std::string;

struct activity_data
{
    database_id m_activityId = 0;
    database_id m_reviewId   = 0;
    database_id m_projectId  = 0;
    database_id m_userId     = 0;
    activity_type m_activityType;
    std::string m_activityText;
    std::string m_activityTimestamp;

    template <class archive_class>
    auto serialize(archive_class& archive, const unsigned int version) -> void
    {
        archive & BOOST_SERIALIZATION_NVP(m_activityId);
        archive & BOOST_SERIALIZATION_NVP(m_reviewId);
        archive & BOOST_SERIALIZATION_NVP(m_projectId);
        archive & BOOST_SERIALIZATION_NVP(m_userId);
        archive & BOOST_SERIALIZATION_NVP(m_activityType);
        archive & BOOST_SERIALIZATION_NVP(m_activityText);
        archive & BOOST_SERIALIZATION_NVP(m_activityTimestamp);
    }
};