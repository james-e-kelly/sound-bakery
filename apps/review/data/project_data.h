#pragma once

#include "pch.h"

#include "workspace_data.h"

struct project_data
{
    database_id m_id = 0;
    std::string m_projectName;
    std::string m_projectDescription;
    int m_openReviews = 0;
    int m_closedReviews = 0;
    int m_archivedReviews = 0;

    template <class archive_class>
    auto serialize(archive_class& archive, const unsigned int version) -> void
    {
        archive & BOOST_SERIALIZATION_NVP(m_id);
        archive & BOOST_SERIALIZATION_NVP(m_projectName);
        archive & BOOST_SERIALIZATION_NVP(m_projectDescription);
        archive & BOOST_SERIALIZATION_NVP(m_openReviews);
        archive & BOOST_SERIALIZATION_NVP(m_closedReviews);
        archive & BOOST_SERIALIZATION_NVP(m_archivedReviews);
    }

    bool operator<(const project_data& project) const
    {
        return m_id < project.m_id;
    }
};