#include "workspace_manager.h"

#include "widgets/intro_widget.h"
#include "widgets/workspace_widget.h"

namespace
{
    static constexpr const char* g_workspaceExtension			= "workspace";
    static constexpr const char* g_workspaceExtensionWithDot	= ".workspace";
}

auto workspace_manager::init(gluten::app* app) -> void
{
	if (!m_workspaceController.workspace_exists())
	{
        m_introWidget = app->get_subsystem_by_class<gluten::widget_subsystem>()->add_widget_class_to_root<intro_widget>(false);
	}
}

auto workspace_manager::start() -> void
{
	if (m_workspaceController.workspace_exists())
	{
        open_workspace(m_workspaceController->m_workspaceFilePath);
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

auto workspace_manager::open_workspace(const std::filesystem::path& workspaceFile) -> void
{
	if (std::filesystem::exists(workspaceFile))
	{
        const std::string extension = workspaceFile.extension().string();

		if (extension == g_workspaceExtensionWithDot)
		{
            workspace_file_data fileData;

			gluten::app::load_data_from_disk(workspaceFile, fileData);

			m_introWidget.reset();

			m_workspaceWidget = get_app()->get_subsystem_by_class<gluten::widget_subsystem>()->add_widget_class_to_root<workspace_widget>(false);

			gluten::dockspace_refresh refresh = get_app()->get_subsystem_by_class<gluten::widget_subsystem>()->get_root_widget()->set_manual_layout();
			refresh.assign_widget_to_node(rttr::type::get<workspace_widget>(), refresh.dockspaceID);

			m_workspaceController.open_workspace(workspaceFile);
		}
	}
}

auto workspace_manager::create_workspace(const std::string& workspaceName, const std::filesystem::path& workspaceDirectory) -> void
{
	if (!workspaceName.empty() && std::filesystem::exists(workspaceDirectory))
	{
        const std::filesystem::path workspaceFile = workspaceDirectory / (workspaceName + g_workspaceExtension);

		std::filesystem::create_directories(workspaceDirectory);

		workspace_file_data fileData;
        fileData.m_workspaceName = workspaceName;

		gluten::app::save_data_to_disk(workspaceFile, fileData);

		m_introWidget.reset();

		m_workspaceWidget = get_app()->get_subsystem_by_class<gluten::widget_subsystem>()->add_widget_class_to_root<workspace_widget>(false);

		gluten::dockspace_refresh refresh = get_app()->get_subsystem_by_class<gluten::widget_subsystem>()->get_root_widget()->set_manual_layout();
		refresh.assign_widget_to_node(rttr::type::get<workspace_widget>(), refresh.dockspaceID);

		m_workspaceController.open_workspace(workspaceFile);
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

		m_workspaceController->m_workspaceFilePath.clear();
	}
}

auto workspace_manager::add_existing_project(const project_data& projectData) -> void
{
    m_projects.push_back(std::make_shared<project_data>(std::move(projectData)));
}

auto workspace_manager::create_project(const std::string& projectName, const std::string& projectDescription) -> void
{
    project_data projectData{.m_projectName = projectName, .m_projectDescription = projectDescription};
    add_existing_project(projectData);

    m_workspaceController.create_project(projectName, projectDescription);
}

auto workspace_manager::get_projects() const -> std::vector<std::shared_ptr<project_data>> { return m_projects; }
