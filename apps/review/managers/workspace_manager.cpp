#include "workspace_manager.h"

#include "widgets/intro_widget.h"
#include "widgets/workspace_widget.h"

namespace
{
    constexpr const char* g_workspaceExtension			= "workspace";
    constexpr const char* g_workspaceExtensionWithDot	= ".workspace";
    constexpr const char* g_projectDirectoryName		= "Projects";
    constexpr const char* g_userDirectoryName			= "Users";
    constexpr const char* g_projectFileExtension		= "project";
}

auto workspace_manager::init(gluten::app* app) -> void
{
    if (!m_userSettingsData->workspace_exists())
	{
        m_introWidget = app->get_subsystem_by_class<gluten::widget_subsystem>()->add_widget_class_to_root<intro_widget>(false);
	}
}

auto workspace_manager::start() -> void
{
    if (m_userSettingsData->workspace_exists())
	{
        open_workspace(m_userSettingsData->m_workspaceFilePath);
	}

	if (m_introWidget)
	{
        gluten::dockspace_refresh refresh = get_app()->get_subsystem_by_class<gluten::widget_subsystem>()->get_root_widget()->set_manual_layout();
		refresh.assign_widget_to_node(rttr::type::get<intro_widget>(), refresh.dockspaceID);
	}

	if (m_workspaceWidget)
	{
        gluten::dockspace_refresh refresh = get_app()->get_subsystem_by_class<gluten::widget_subsystem>()->get_root_widget()->set_manual_layout();
        refresh.assign_widget_to_node(rttr::type::get<workspace_widget>(), refresh.dockspaceID);
	}
}

auto workspace_manager::get_projects_directory() const -> std::filesystem::path
{
    return m_userSettingsData->m_workspaceFilePath.parent_path() / g_projectDirectoryName;
}

auto workspace_manager::get_user_directory() const -> std::filesystem::path
{
    return m_userSettingsData->m_workspaceFilePath.parent_path() / g_userDirectoryName;
}

auto workspace_manager::open_workspace(const std::filesystem::path& workspaceFile) -> void
{
    if (file_is_workspace(workspaceFile))
    {
        m_introWidget.reset();

        m_userSettingsData->m_workspaceFilePath = workspaceFile;

        workspace_data fileData;
        gluten::app::load_data_from_disk(workspaceFile, fileData);

        m_workspaceWidget = get_app()->get_subsystem_by_class<gluten::widget_subsystem>()->add_widget_class_to_root<workspace_widget>(false);

        gluten::dockspace_refresh refresh =
            get_app()->get_subsystem_by_class<gluten::widget_subsystem>()->get_root_widget()->set_manual_layout();
        refresh.assign_widget_to_node(rttr::type::get<workspace_widget>(), refresh.dockspaceID);
    
        load_projects_from_workspace();
    }
}

auto workspace_manager::load_projects_from_workspace() -> void
{
    if (std::filesystem::exists(get_projects_directory()))
    {
        for (const auto& dir : std::filesystem::directory_iterator(get_projects_directory()))
        {
            if (dir.is_directory())
            {
                const std::filesystem::path projectDirectory = dir.path();
                const std::string projectId                  = projectDirectory.stem().string();
                const std::filesystem::path projectFile =
                    (projectDirectory / projectId).replace_extension(g_projectFileExtension);

                if (std::filesystem::exists(projectFile))
                {
                    project_data projectData;
                    gluten::app::load_data_from_disk(projectFile, projectData);

                    add_existing_project(projectData);
                }
            }
        }
    }
}

auto workspace_manager::create_project(const std::string& projectName, const std::string& projectDescription) -> void
{
    project_data projectData;
    projectData.m_projectName        = projectName;
    projectData.m_projectDescription = projectDescription;

    const std::filesystem::path projectFile =
        (get_projects_directory() / projectName / projectName).replace_extension(g_projectFileExtension);

    gluten::app::save_data_to_disk(projectFile, projectData);

    m_projects.push_back(std::make_shared<project_data>(std::move(projectData)));
}

auto workspace_manager::create_workspace(const std::string& workspaceName, const std::filesystem::path& workspaceDirectory) -> void
{
	if (!workspaceName.empty() && std::filesystem::exists(workspaceDirectory))
	{
        const std::filesystem::path workspaceFile = workspaceDirectory / (workspaceName + g_workspaceExtension);

		std::filesystem::create_directories(workspaceDirectory);

		workspace_data fileData;
        fileData.m_workspaceName = workspaceName;

		gluten::app::save_data_to_disk(workspaceFile, fileData);

		m_introWidget.reset();

		m_workspaceWidget = get_app()->get_subsystem_by_class<gluten::widget_subsystem>()->add_widget_class_to_root<workspace_widget>(false);

		gluten::dockspace_refresh refresh = get_app()->get_subsystem_by_class<gluten::widget_subsystem>()->get_root_widget()->set_manual_layout();
		refresh.assign_widget_to_node(rttr::type::get<workspace_widget>(), refresh.dockspaceID);
	}
}

auto workspace_manager::close_workspace() -> void
{
	m_workspaceWidget.reset();

	if (!m_introWidget)
	{
		m_introWidget = get_app()->get_subsystem_by_class<gluten::widget_subsystem>()->add_widget_class_to_root<intro_widget>(false);

		gluten::dockspace_refresh refresh = get_app()->get_subsystem_by_class<gluten::widget_subsystem>()->get_root_widget()->set_manual_layout();
        refresh.assign_widget_to_node(rttr::type::get<intro_widget>(), refresh.dockspaceID);

        m_userSettingsData->m_workspaceFilePath.clear();
	}
}

auto workspace_manager::add_existing_project(const project_data& projectData) -> void
{
    m_projects.push_back(std::make_shared<project_data>(std::move(projectData)));
}

auto workspace_manager::get_projects() const -> std::vector<std::shared_ptr<project_data>> { return m_projects; }

auto workspace_manager::get_workspace_file() const -> std::filesystem::path
{
    return m_userSettingsData->m_workspaceFilePath;
}

auto workspace_manager::get_workspace_directory() const -> std::filesystem::path
{
    return m_userSettingsData->m_workspaceFilePath.parent_path();
}

auto workspace_manager::file_is_workspace(const std::filesystem::path& file) -> bool
{
    return std::filesystem::exists(file) && file.extension().string() == g_workspaceExtensionWithDot;
}