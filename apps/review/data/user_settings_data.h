#pragma once

#include "pch.h"

/**
 * @brief Cached user settings like the open workspace
 *
 * Stores where the workspace lives so it can be loaded upon boot automatically.
 */
struct user_settings_data
{
    std::filesystem::path m_workspaceFilePath;  //< Top most file that will be used to find reviews and data in
                                                // subfolders
    std::string m_selectedProjectName;

    auto workspace_exists() const -> bool
    {
        return std::filesystem::exists(m_workspaceFilePath);
    }

    template <class archive_class>
    auto serialize(archive_class& archive, const unsigned int version) -> void
    {
        archive& boost::serialization::make_nvp("Workspace", m_workspaceFilePath);

        if (version >= review_app_save_selected_project)
        {
            archive& boost::serialization::make_nvp("SelectedProject", m_selectedProjectName);
        }
    }
};

BOOST_CLASS_VERSION(user_settings_data, review_app_version_current)