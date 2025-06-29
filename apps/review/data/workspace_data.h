#pragma once

#include "pch.h"

#include "gluten/data/data_controller.h"

using project_id_type = unsigned int;

enum workspace_data_version
{
    workspace_version_start = 0,

    // Add versions above this //
    workspace_version_end,
    workspace_version_current = workspace_version_end - 1
};

/**
 * @brief Data saved into the user preferences folder.
 * 
 * Stores where the workspace lives so it can be loaded upon boot automatically.
 */
struct workspace_cache_data
{
    std::filesystem::path m_workspaceFilePath;  //< Top most file that will be used to find reviews and data in
                                                //subfolders

    template <class archive_class>
    auto serialize(archive_class& archive, const unsigned int version) -> void
    {
        archive & boost::serialization::make_nvp("Workspace", m_workspaceFilePath);
    }
};

struct workspace_cache_controller : public gluten::data_controller<workspace_cache_data>
{
    auto workspace_exists() const -> bool;

    auto get_projects_directory() const -> std::filesystem::path;   //< Directory that contains each project. Projects contain reviews
    auto get_user_directory() const -> std::filesystem::path;       //< Directory that contains user data like profile pictures, emails, names etc.

    auto open_workspace(const std::filesystem::path& workspaceFile) -> void;    //< Store the workspace file and load all data like projects.

    auto create_project(const std::string& projectName, const std::string& projectDescription) -> void;
};

/**
 * @brief Saves data about the workspace into the public filesystem used by everyone.
 */
struct workspace_file_data
{
    std::string m_workspaceName;

    template <class archive_class>
    auto serialize(archive_class& archive, unsigned int fileVersion)
    {
        archive & boost::serialization::make_nvp("name", m_workspaceName);
    }
};

BOOST_CLASS_VERSION(workspace_cache_data, workspace_version_current)
BOOST_CLASS_VERSION(workspace_file_data, workspace_version_current)