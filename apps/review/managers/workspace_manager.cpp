#include "workspace_manager.h"

#include "app/review_database.h"
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

workspace_manager::~workspace_manager()
{
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

auto workspace_manager::open_workspace(const std::filesystem::path& workspaceFile) -> void
{
    if (file_is_workspace(workspaceFile))
    {
        m_introWidget.reset();

        m_userSettingsData->m_workspaceFilePath = workspaceFile;

        m_database = std::make_shared<review_database>(std::filesystem::path(workspaceFile).replace_extension("db"));
        m_database->open_workspace(workspaceFile.stem().string()).get();
        m_database->get_workspace_name().get();

        m_workspaceWidget = get_app()->get_subsystem_by_class<gluten::widget_subsystem>()->add_widget_class_to_root<workspace_widget>(false);

        gluten::dockspace_refresh refresh =
            get_app()->get_subsystem_by_class<gluten::widget_subsystem>()->get_root_widget()->set_manual_layout();
        refresh.assign_widget_to_node(rttr::type::get<workspace_widget>(), refresh.dockspaceID);
    
        load_projects_from_workspace();

        if (!m_userSettingsData->m_selectedProjectName.empty())
        {
            select_project(m_userSettingsData->m_selectedProjectName);
        }
    }
}

auto workspace_manager::load_projects_from_workspace() -> void
{
    const std::vector<project_data> allProjects = m_database->get_all_projects().get();
    m_projects = std::set<project_data>(allProjects.cbegin(), allProjects.cend());
}

auto workspace_manager::create_project(const std::string& projectName, const std::string& projectDescription) -> void
{
    m_projects.insert(m_database->create_project(projectName, projectDescription).get());
}

auto workspace_manager::select_project(const std::string& projectName) -> void
{
    if (projectName.empty())
    {
        m_selectedProject = project_data();
        m_selectedReview  = review_data();
        m_reviews.clear();
        return;
    }

    for (const auto& project : m_projects)
    {
        if (project.m_projectName == projectName)
        {
            m_selectedProject = project;

            std::vector<review_data> allReviews = m_database->get_all_reviews(m_selectedProject.m_id).get();
            m_reviews         = std::set<review_data>(allReviews.cbegin(), allReviews.cend());
            break;
        }
    }
}

auto workspace_manager::has_selected_project() const -> bool { return m_selectedProject.m_id != 0; }

auto workspace_manager::create_workspace(const std::string& workspaceName, const std::filesystem::path& workspaceDirectory) -> void
{
	if (!workspaceName.empty() && std::filesystem::exists(workspaceDirectory))
	{
        const std::filesystem::path workspaceFile = workspaceDirectory / (workspaceName + g_workspaceExtension);
		std::filesystem::create_directories(workspaceDirectory);

        m_database = std::make_shared<review_database>(std::filesystem::path(workspaceFile).replace_extension("db"));
        m_database->create_workspace(workspaceName);

		m_introWidget.reset();

		m_workspaceWidget = get_app()->get_subsystem_by_class<gluten::widget_subsystem>()->add_widget_class_to_root<workspace_widget>(false);

		gluten::dockspace_refresh refresh = get_app()->get_subsystem_by_class<gluten::widget_subsystem>()->get_root_widget()->set_manual_layout();
		refresh.assign_widget_to_node(rttr::type::get<workspace_widget>(), refresh.dockspaceID);
	}
}

auto workspace_manager::close_workspace() -> void
{
	m_workspaceWidget.reset();
    m_selectedProject = project_data();
    m_projects.clear();
    m_userSettingsData->m_workspaceFilePath.clear();
    m_database.reset();

	if (!m_introWidget)
	{
		m_introWidget = get_app()->get_subsystem_by_class<gluten::widget_subsystem>()->add_widget_class_to_root<intro_widget>(false);

		gluten::dockspace_refresh refresh = get_app()->get_subsystem_by_class<gluten::widget_subsystem>()->get_root_widget()->set_manual_layout();
        refresh.assign_widget_to_node(rttr::type::get<intro_widget>(), refresh.dockspaceID);
	}
}

auto workspace_manager::add_existing_project(const project_data& projectData) -> void
{
    m_projects.insert(projectData);
}

auto workspace_manager::get_selected_project() const -> const project_data&
{
    return m_selectedProject;
}

auto workspace_manager::get_projects() const -> const std::set<project_data>& { return m_projects; }

auto workspace_manager::get_workspace_name() const -> std::string
{
    return m_database->get_workspace_name().get();
}

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

auto workspace_manager::select_review(int64_t reviewId) -> void
{
    if (reviewId == 0)
    {
        m_selectedReview  = review_data();
        return;
    }

    for (const auto& review : m_reviews)
    {
        if (review.m_reviewId == reviewId)
        {
            m_selectedReview = review;
            break;
        }
    }
}

auto workspace_manager::get_selected_review() const -> const review_data&
{
    return m_selectedReview;
}

auto workspace_manager::create_review(const new_review_data& newReview) -> void 
{
    m_reviews.insert(m_database->create_review(get_selected_project().m_id, newReview).get());
}

auto workspace_manager::update_review(const review_data& updatedReview) -> void
{
    m_reviews.erase(std::find_if(m_reviews.begin(), m_reviews.end(), [id = updatedReview.m_reviewId](const review_data& review){ return review.m_reviewId == id; }));
    m_reviews.insert(m_database->update_review(updatedReview).get());
    
    if (m_selectedReview.m_reviewId == updatedReview.m_reviewId)
    {
        m_selectedReview = updatedReview;
    }
}

auto workspace_manager::get_all_reviews() const -> const std::set<review_data>&
{
    return m_reviews;
}