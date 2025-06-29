#include "workspace_data.h"

#include "data/project_data.h"
#include "managers/workspace_manager.h"

namespace
{
    constexpr const char* g_projectDirectoryName = "Projects";
    constexpr const char* g_userDirectoryName    = "Users";
    constexpr const char* g_projectFileExtension = "project";
}

auto workspace_cache_controller::workspace_exists() const -> bool 
{
    return std::filesystem::exists(get_const_data()->m_workspaceFilePath);
}

auto workspace_cache_controller::get_projects_directory() const -> std::filesystem::path
{
    return get_const_data()->m_workspaceFilePath.parent_path() / g_projectDirectoryName;
}

auto workspace_cache_controller::get_user_directory() const -> std::filesystem::path
{
    return get_const_data()->m_workspaceFilePath.parent_path() / g_userDirectoryName;
}

auto workspace_cache_controller::open_workspace(const std::filesystem::path& workspaceFile) -> void
{
    get_data()->m_workspaceFilePath = workspaceFile;

    if (std::filesystem::exists(get_projects_directory()))
    {
        for (const auto& dir : std::filesystem::directory_iterator(get_projects_directory()))
        {
            if (dir.is_directory())
            {
                const std::filesystem::path projectDirectory = dir.path();
                const std::string projectId                  = projectDirectory.stem().string();
                const std::filesystem::path projectFile      = (projectDirectory / projectId).replace_extension(g_projectFileExtension);

                if (std::filesystem::exists(projectFile))
                {
                    project_data projectData;
                    gluten::app::load_data_from_disk(projectFile, projectData);

                    if (std::shared_ptr<workspace_manager> workspaceManager = gluten::app::get()->get_manager_by_class<workspace_manager>())
                    {
                        workspaceManager->add_existing_project(projectData);
                    }
                }
            }
        }
    }
}

auto workspace_cache_controller::create_project(const std::string& projectName,
                                                      const std::string& projectDescription) -> void
{
    project_data projectData;
    projectData.m_projectName        = projectName;
    projectData.m_projectDescription = projectDescription;

    const std::filesystem::path projectFile =
        (get_projects_directory() / projectName / projectName)
            .replace_extension(g_projectFileExtension);

    gluten::app::save_data_to_disk(projectFile, projectData);
}