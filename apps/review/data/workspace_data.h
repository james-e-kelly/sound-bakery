#pragma once

#include "pch.h"

struct workspace_data
{
    std::string m_workspaceName;

    template <class archive_class>
    auto serialize(archive_class& archive, unsigned int fileVersion)
    {
        archive & boost::serialization::make_nvp("name", m_workspaceName);
    }
};

BOOST_CLASS_VERSION(workspace_data, review_app_version_current)