#include "workspace_manager.h"

#include "widgets/intro_widget.h"
#include "widgets/user_flow_popup.h"
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

        m_database = std::make_shared<review_database>(std::filesystem::path(workspaceFile));
        m_database->open_workspace(workspaceFile.stem().string()).get();
        m_database->get_workspace_name().get();

        if (m_database->user_table_is_empty().get() || m_userSettingsData->m_loggedInUser.m_sessionToken.empty())
        {
            open_user_flow_popup();
        }
        else
        {
            if (m_database->user_is_logged_in_and_has_privilege_for_action(m_userSettingsData->m_loggedInUser.m_sessionToken, activity_type::review_created).get())
            {
                open_workspace_widget();
            }
            else
            {
                open_user_flow_popup();
            }
        }
    }
}

auto workspace_manager::load_projects_from_workspace() -> void
{
    const std::vector<project_data> allProjects = m_database->get_all_projects().get();
    m_projects = std::set<project_data>(allProjects.cbegin(), allProjects.cend());
}

auto workspace_manager::open_workspace_widget() -> void 
{
    m_userFlowPopup.reset();
    m_workspaceWidget = get_app()->get_subsystem_by_class<gluten::widget_subsystem>()->add_widget_class_to_root<workspace_widget>(false);

    gluten::dockspace_refresh refresh = get_app()->get_subsystem_by_class<gluten::widget_subsystem>()->get_root_widget()->set_manual_layout();
    refresh.assign_widget_to_node(rttr::type::get<workspace_widget>(), refresh.dockspaceID);

    load_projects_from_workspace();

    if (!m_userSettingsData->m_selectedProjectName.empty())
    {
        select_project(m_userSettingsData->m_selectedProjectName);
    }
}

auto workspace_manager::open_user_flow_popup() -> void
{
    m_workspaceWidget.reset();
    m_userFlowPopup = get_app()->get_subsystem_by_class<gluten::widget_subsystem>()->add_widget_class_to_root<user_flow_popup>(false);
    m_userFlowPopup->open_popup();

    if (m_database->user_table_is_empty().get())
    {
        m_userFlowPopup->set_flow_type(user_flow_type::new_user_and_login);
    }
    else if (m_userSettingsData->m_loggedInUser.m_sessionToken.empty())
    {
        m_userFlowPopup->set_flow_type(user_flow_type::login_user);
    }
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
        const std::filesystem::path workspaceFile = workspaceDirectory / (workspaceName + g_workspaceExtensionWithDot);
		std::filesystem::create_directories(workspaceDirectory);

        m_database = std::make_shared<review_database>(workspaceFile);
        m_database->create_workspace(workspaceName);

		m_introWidget.reset();

		open_user_flow_popup();
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

auto workspace_manager::get_all_activity_for_review(int64_t reviewId) const -> const std::vector<activity_data>&
{
    static cache<activity_data> s_cachedActivity;

    if (s_cachedActivity.check_for_cache_invalidation(reviewId, get_activity_count_for_review(reviewId)))
    {
        s_cachedActivity.add_cache(reviewId, m_database->get_all_activity_for_review(reviewId));
    }

    return s_cachedActivity.get_cache(reviewId);
}

auto workspace_manager::get_activity_count_for_review(int64_t reviewId) const -> concurrencpp::result<std::size_t>
{
    co_return co_await m_database->get_activity_count_for_review(reviewId);
}

auto workspace_manager::get_all_comments_for_review(int64_t reviewId) -> const std::vector<comment_data>&
{
    if (m_cachedComments.check_for_cache_invalidation(reviewId, get_comments_count_for_review(reviewId)))
    {
        m_cachedComments.add_cache(reviewId, m_database->get_all_comments_for_review(reviewId));
    }
    
    return m_cachedComments.get_cache(reviewId);
}

auto workspace_manager::get_comments_count_for_review(int64_t reviewId) const -> concurrencpp::result<std::size_t>
{
    co_return co_await m_database->get_comments_count_for_review(reviewId);
}

auto workspace_manager::create_comment(const new_comment_data& newComment) -> void
{
    m_database->create_comment(newComment).get();
}

auto workspace_manager::delete_comment(int64_t commentId) -> void
{
    m_database->delete_comment(commentId).get();
}

auto workspace_manager::open_create_user_popup() -> void
{
    // New users are only added once the workspace is open
    if (m_workspaceWidget)
    {
        m_userFlowPopup = get_app()->get_subsystem_by_class<gluten::widget_subsystem>()->add_widget_class_to_root<user_flow_popup>(false);
        m_userFlowPopup->open_popup();
        m_userFlowPopup->set_flow_type(user_flow_type::new_user);
    }
}

auto workspace_manager::users_table_is_empty() -> concurrencpp::result<bool>
{
    co_return co_await m_database->user_table_is_empty();
}

auto workspace_manager::logged_in_user_can_create_users() -> concurrencpp::result<bool>
{
    co_return co_await m_database->user_is_logged_in_and_has_privilege_for_action(m_userSettingsData->m_loggedInUser.m_sessionToken, activity_type::user_added);
}

auto workspace_manager::create_user(const new_user_data newUser, std::optional<std::string> userToken) -> concurrencpp::result<tl::expected<bool, database_error>> 
{
    co_await m_database->create_user(newUser, userToken.has_value() ? userToken.value() : std::string());
    m_userFlowPopup.reset();
}

auto workspace_manager::create_user_and_login(new_user_data newUser) -> concurrencpp::result<tl::expected<bool, database_error>>
{
    if (co_await m_database->user_table_is_empty())
    {
        newUser.m_requestedPrivileges = user_privileges::admin;
    }

    co_await m_database->create_user(newUser, std::string());

    login_request_data loginRequest;
    loginRequest.m_email = newUser.m_email;
    loginRequest.m_rawPassword = newUser.m_rawPassword;

    co_return co_await login_user(loginRequest);
}

auto workspace_manager::login_user(login_request_data loginData) -> concurrencpp::result<tl::expected<bool, database_error>>
{
    const tl::expected<logged_in_user_data, database_error> loggedInUser = co_await m_database->login_user(loginData);

    co_await concurrencpp::resume_on(get_app()->get_tick_executor());

    if (loggedInUser.has_value())
    {
        m_userSettingsData->m_loggedInUser = loggedInUser.value();
        m_userFlowPopup.reset();
        open_workspace_widget();
        co_return true;
    }
    else
    {
        co_return tl::make_unexpected(loggedInUser.error());
    }
}

auto workspace_manager::logout() -> void
{
    m_userSettingsData->m_loggedInUser = logged_in_user_data();
    m_workspaceWidget.reset();
    m_userFlowPopup.reset();

    open_user_flow_popup();
}

auto workspace_manager::get_all_users() -> const std::vector<user_data>&
{
    if (m_allUsersCache.check_for_cache_invalidation(0, get_users_count()))
    {
        m_allUsersCache.add_cache_handle_expected(0, m_database->get_all_users(m_userSettingsData->m_loggedInUser.m_sessionToken));
    }

    return m_allUsersCache.get_cache(0);
}

auto workspace_manager::get_users_count() const -> concurrencpp::result<std::size_t>
{
    const tl::expected<std::size_t, database_error> result = co_await m_database->get_users_count(m_userSettingsData->m_loggedInUser.m_sessionToken);

    if (result.has_value())
    {
        co_return result.value();
    }

    co_return 0U;
}

auto workspace_manager::get_selected_user() const -> const user_data&
{
    return m_selectedUser;
}

auto workspace_manager::select_user(const std::string& email) -> void
{
    if (email.empty())
    {
        m_selectedUser = user_data();
        return;
    }

    for (const auto& user : m_allUsersCache.get_cache(0))
    {
        if (user.m_email == email)
        {
            m_selectedUser = user;
            break;
        }
    }
}

auto workspace_manager::delete_user(const std::string& email) -> void
{
    if (!email.empty())
    {
        const bool deletingSelf = email == m_userSettingsData->m_loggedInUser.m_email;

        const tl::expected<bool, database_error> result = m_database->delete_user(email, m_userSettingsData->m_loggedInUser.m_sessionToken).get();

        if (result.has_value())
        {
            if (result.value())
            {
                m_selectedProject = project_data();
                m_selectedReview  = review_data();
                m_selectedUser    = user_data();

                if (deletingSelf)
                {
                    logout();
                }
            }
        }
    }
}

auto workspace_manager::get_users_for_review(int64_t reviewId) -> const std::vector<reviewer_data>&
{
    if (m_reviewUsersCache.check_for_cache_invalidation(reviewId, get_users_count()))
    {
        const tl::expected<std::vector<user_data>, database_error> result = m_database->get_users_for_review(reviewId, m_userSettingsData->m_loggedInUser.m_sessionToken).get();

        if (result.has_value())
        {
            std::vector<user_data> users = result.value();
            std::vector<reviewer_data> reviewers;
            reviewers.resize(users.size());

            std::transform(users.begin(), users.end(), reviewers.begin(), [database = m_database, reviewId](const user_data& user) 
                {
                    return reviewer_data(user, database->get_user_vote_on_review(reviewId, user.m_userId).get().value());
                });

            std::sort(reviewers.begin(), reviewers.end(), [userSettings = m_userSettingsData](const reviewer_data& lhs, const reviewer_data& rhs) -> bool
                {
                    return std::strcmp(userSettings->m_loggedInUser.m_email.c_str(), lhs.m_email.c_str()) == 0;
                });

            std::sort(reviewers.begin() + 1, reviewers.end(), [](const reviewer_data& lhs, const reviewer_data& rhs) 
                {
                    return std::strcmp(lhs.m_displayName.c_str(), rhs.m_displayName.c_str()) < 0;
                });

            m_reviewUsersCache.add_cache_sync(reviewId, reviewers);
        }
    }

    return m_reviewUsersCache.get_cache(reviewId);
}

auto workspace_manager::set_users_for_review(int64_t reviewId, std::vector<int64_t> userIds) -> concurrencpp::result<tl::expected<bool, database_error>>
{
    m_reviewUsersCache.erase_cache(reviewId);

    co_return co_await m_database->set_users_for_review(reviewId, userIds, m_userSettingsData->m_loggedInUser.m_sessionToken);
}

auto workspace_manager::set_user_vote_for_review(int64_t reviewId, int64_t userId, review_vote vote) -> concurrencpp::result<void>
{
    gluten::data_source<user_settings_data> userSettings;

    co_await m_database->set_user_vote_for_review(reviewId, userId, vote, userSettings->m_loggedInUser.m_sessionToken);

    co_await concurrencpp::resume_on(gluten::app::get()->get_tick_executor());

    m_reviewUsersCache.erase_cache(reviewId);
}