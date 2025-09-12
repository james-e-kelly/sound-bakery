#pragma once

#include "pch.h"

#include "workspace_data.h"

struct project_data
{
    database_id m_id = 0;
    std::string m_projectName;
    std::string m_projectDescription;

    template <class archive_class>
    auto serialize(archive_class& archive, const unsigned int version) -> void
    {
        archive & boost::serialization::make_nvp("project_name", m_projectName);
        archive & boost::serialization::make_nvp("project_description", m_projectDescription);
    }

    bool operator<(const project_data& project) const
    {
        return m_id < project.m_id;
    }
};