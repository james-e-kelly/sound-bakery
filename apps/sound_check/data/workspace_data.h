#pragma once

#include "pch.h"

#include "gluten/data/data_controller.h"

struct workspace_data
{
    std::filesystem::path m_workspaceFilePath;  //< Top most file that will be used to find reviews and data in
                                                //subfolders

    template <class archive_class>
    auto serialize(archive_class& archive, const unsigned int version) -> void
    {
        archive & boost::serialization::make_nvp("Workspace", m_workspaceFilePath);
    }
};

struct workspace_controller : public gluten::data_controller<workspace_data>
{
    auto workspace_exists() const -> bool;

    auto get_projects_directory() const -> std::filesystem::path;   //< Directory that contains each project. Projects contain reviews
    auto get_user_directory() const -> std::filesystem::path;       //< Directory that contains user data like profile pictures, emails, names etc.
};