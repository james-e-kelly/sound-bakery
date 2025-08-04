#include "workspace_manager.h"

#include "app/review_app.h"
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

auto workspace_manager::open_workspace(const std::filesystem::path workspaceFile) -> concurrencpp::result<void>
{
    co_await concurrencpp::resume_on(get_app()->get_tick_executor());

    if (file_is_workspace(workspaceFile))
    {
        m_introWidget.reset();

        m_userSettingsData->m_workspaceFilePath = workspaceFile;

        const std::shared_ptr<gluten::loading_popup> loadingPopup = get_app()->get_subsystem_by_class<gluten::widget_subsystem>()->add_widget_class_to_root<gluten::loading_popup>(false);
        loadingPopup->open_popup();

        m_database = std::make_shared<review_database>(std::filesystem::path(workspaceFile));
        
        co_await concurrencpp::resume_on(review_app::get()->thread_pool_executor());
        co_await m_database->open_workspace(workspaceFile.stem().string());
        co_await m_database->get_workspace_name();

        if (co_await m_database->user_table_is_empty() || m_userSettingsData->m_loggedInUser.m_sessionToken.empty())
        {
            co_await concurrencpp::resume_on(review_app::get()->get_tick_executor());
            open_user_flow_popup();
        }
        else
        {
            if (co_await m_database->user_is_logged_in_and_has_privilege_for_action(m_userSettingsData->m_loggedInUser.m_sessionToken, activity_type::review_created))
            {
                co_await concurrencpp::resume_on(review_app::get()->get_tick_executor());
                open_workspace_widget();
            }
            else
            {
                co_await concurrencpp::resume_on(review_app::get()->get_tick_executor());
                open_user_flow_popup();
            }
        }
    }
}

auto workspace_manager::async_get_users_for_review(int64_t reviewId) -> concurrencpp::result<std::vector<reviewer_data>>
{
    const tl::expected<std::vector<user_data>, database_error> result = co_await m_database->get_users_for_review(reviewId, m_userSettingsData->m_loggedInUser.m_sessionToken);

    std::vector<reviewer_data> reviewers;

    assert(result.has_value());

    if (result.has_value())
    {
        std::vector<user_data> users = result.value();
        reviewers.resize(users.size());

        if (!users.empty() && !reviewers.empty())
        {
            co_await concurrencpp::resume_on(review_app::get()->thread_pool_executor());

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
        }

    }
    co_return reviewers;
}

auto workspace_manager::open_workspace_widget() -> void 
{
    m_userFlowPopup.reset();
    m_workspaceWidget = get_app()->get_subsystem_by_class<gluten::widget_subsystem>()->add_widget_class_to_root<workspace_widget>(false);

    gluten::dockspace_refresh refresh = get_app()->get_subsystem_by_class<gluten::widget_subsystem>()->get_root_widget()->set_manual_layout();
    refresh.assign_widget_to_node(rttr::type::get<workspace_widget>(), refresh.dockspaceID);

    if (!m_userSettingsData->m_selectedProjectName.empty())
    {
        select_project(m_userSettingsData->m_selectedProjectName);
    }
}

auto workspace_manager::open_user_flow_popup() -> concurrencpp::result<void>
{
    co_await concurrencpp::resume_on(get_app()->get_tick_executor());

    m_workspaceWidget.reset();
    m_userFlowPopup = get_app()->get_subsystem_by_class<gluten::widget_subsystem>()->add_widget_class_to_root<user_flow_popup>(false);
    m_userFlowPopup->open_popup();

    if (co_await m_database->user_table_is_empty())
    {
        co_await concurrencpp::resume_on(get_app()->get_tick_executor());
        m_userFlowPopup->set_flow_type(user_flow_type::new_user_and_login);
    }
    
    co_await concurrencpp::resume_on(get_app()->get_tick_executor());

    if (m_userSettingsData->m_loggedInUser.m_sessionToken.empty())
    {
        m_userFlowPopup->set_flow_type(user_flow_type::login_user);
    }
}

auto workspace_manager::create_project(const std::string& projectName, const std::string& projectDescription) -> void
{
    m_database->create_project(projectName, projectDescription);
    m_cachedProjects.set_cache_expired(m_userSettingsData->m_loggedInUser.m_sessionToken);
}

auto workspace_manager::select_project(const std::string projectName) -> concurrencpp::result<void>
{
    if (projectName.empty())
    {
        m_selectedProject = project_data();
        m_selectedReview  = review_data();
        co_return;
    }

    for (const auto& project : m_cachedProjects.get_cached_data(gluten::token_cache_key(m_userSettingsData->m_loggedInUser.m_sessionToken)).m_cache)
    {
        if (project.m_projectName == projectName)
        {
            m_selectedProject = project;

            const gluten::key_and_token_cache_key key(m_selectedProject.m_id, m_userSettingsData->m_loggedInUser.m_sessionToken);

            if (m_cachedReviews.get_cache_needs_filling(key))
            {
                m_cachedReviews.set_async_fill_cache(key, m_database->get_all_reviews(key.m_key));
            }
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
    m_cachedProjects.clear();
    m_userSettingsData->m_workspaceFilePath.clear();
    m_database.reset();

	if (!m_introWidget)
	{
		m_introWidget = get_app()->get_subsystem_by_class<gluten::widget_subsystem>()->add_widget_class_to_root<intro_widget>(false);

		gluten::dockspace_refresh refresh = get_app()->get_subsystem_by_class<gluten::widget_subsystem>()->get_root_widget()->set_manual_layout();
        refresh.assign_widget_to_node(rttr::type::get<intro_widget>(), refresh.dockspaceID);
	}
}

auto workspace_manager::get_selected_project() const -> const project_data&
{
    return m_selectedProject;
}

auto workspace_manager::get_all_projects() -> typename global_cache_type<project_data>::cache_result
{
    const gluten::token_cache_key key(m_userSettingsData->m_loggedInUser.m_sessionToken);

    if (m_cachedProjects.get_cache_needs_filling(key))
    {
        m_cachedProjects.set_async_fill_cache(key, m_database->get_all_projects());
        m_selectedProject = project_data();
    }

    return m_cachedProjects.get_cached_data(gluten::token_cache_key<std::string>(m_userSettingsData->m_loggedInUser.m_sessionToken));
}

auto workspace_manager::get_workspace_name() -> typename string_cache_type::cache_result
{
    const gluten::token_cache_key key(m_userSettingsData->m_loggedInUser.m_sessionToken);

    if (m_cachedWorkspaceName.get_cache_needs_filling(key))
    {
        m_cachedWorkspaceName.set_async_fill_cache(key, m_database->get_workspace_name());
    }

    return m_cachedWorkspaceName.get_cached_data(key);
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

    for (const auto& review : m_cachedReviews.get_cached_data(gluten::key_and_token_cache_key<int64_t, std::string>(m_selectedProject.m_id, m_userSettingsData->m_loggedInUser.m_sessionToken)).m_cache)
    {
        if (review.m_reviewId == reviewId)
        {
            m_selectedReview = review;
            get_all_comments_for_review(m_selectedReview.m_reviewId);
            get_all_activity_for_review(m_selectedReview.m_reviewId);
            get_users_for_review(m_selectedReview.m_reviewId);
            break;
        }
    }
}

auto workspace_manager::get_selected_review() const -> const review_data&
{
    return m_selectedReview;
}

auto workspace_manager::create_review(const new_frontend_review_data newReview) -> concurrencpp::result<void> 
{
    co_await concurrencpp::resume_on(get_app()->background_executor());

    new_transit_review_data newBackendReviewData(newReview);

    const auto createReviewResult = co_await m_database->create_review(get_selected_project().m_id, newBackendReviewData);

    co_await concurrencpp::resume_on(get_app()->get_tick_executor());

    m_cachedReviews.set_cache_expired({m_selectedProject.m_id, m_userSettingsData->m_loggedInUser.m_sessionToken});
}

auto workspace_manager::update_review(const review_data& updatedReview) -> void
{
    m_cachedReviews.set_cache_expired({m_selectedProject.m_id, m_userSettingsData->m_loggedInUser.m_sessionToken});
    
    if (m_selectedReview.m_reviewId == updatedReview.m_reviewId)
    {
        m_selectedReview = updatedReview;
    }
}

auto workspace_manager::get_all_reviews() -> typename default_cache_type<review_data>::cache_result
{
    const gluten::key_and_token_cache_key key(m_selectedProject.m_id, m_userSettingsData->m_loggedInUser.m_sessionToken);

    if (m_cachedReviews.get_cache_needs_filling(key))
    {
        m_cachedReviews.set_async_fill_cache(key, m_database->get_all_reviews(m_selectedProject.m_id));
        m_selectedReview = review_data();
    }

    return m_cachedReviews.get_cached_data({m_selectedProject.m_id, m_userSettingsData->m_loggedInUser.m_sessionToken});
}

auto workspace_manager::delete_review(int64_t reviewId) -> concurrencpp::result<void>
{
    const gluten::key_and_token_cache_key key(m_selectedProject.m_id, m_userSettingsData->m_loggedInUser.m_sessionToken);

    m_selectedReview = review_data();
    auto& rawReviews = m_cachedReviews.get_raw_data(key);
    rawReviews.erase(std::find_if(rawReviews.begin(), rawReviews.end(), [reviewId](const review_data& review) 
        {
            return reviewId == review.m_reviewId;
        }));

    co_await m_database->delete_review(reviewId, m_userSettingsData->m_loggedInUser.m_sessionToken);

    m_cachedReviews.set_cache_expired(key);
}

template<typename data_type>
static auto transform_database_result_to_cache_result(concurrencpp::result<tl::expected<data_type, database_error>> databaseResult) -> concurrencpp::result<data_type>
{
    const auto result = co_await databaseResult;

    if (result.has_value())
    {
        co_return result.value();
    }

    co_return data_type();
}

auto workspace_manager::get_all_activity_for_review(int64_t reviewId) -> typename default_cache_type<activity_data>::cache_result
{
    const gluten::key_and_token_cache_key key(reviewId, m_userSettingsData->m_loggedInUser.m_sessionToken);

    if (m_cachedActivity.get_cache_needs_filling(key))
    {
        m_cachedActivity.set_async_fill_cache(key, m_database->get_all_activity_for_review(reviewId));
    }

    return m_cachedActivity.get_cached_data(key);
}

auto workspace_manager::get_all_comments_for_review(int64_t reviewId) -> typename default_cache_type<comment_data>::cache_result
{
    const gluten::key_and_token_cache_key key(reviewId, m_userSettingsData->m_loggedInUser.m_sessionToken);

    if (m_cachedComments.get_cache_needs_filling(key))
    {
        m_cachedComments.set_async_fill_cache(key, m_database->get_all_comments_for_review(reviewId));
    }
    
    return m_cachedComments.get_cached_data(key);
}

auto workspace_manager::create_comment(const new_comment_data& newComment) -> void
{
    m_database->create_comment(newComment);
    m_cachedComments.set_cache_expired({m_selectedProject.m_id, m_userSettingsData->m_loggedInUser.m_sessionToken});
    m_cachedActivity.set_cache_expired({m_selectedReview.m_reviewId, m_userSettingsData->m_loggedInUser.m_sessionToken});
}

auto workspace_manager::delete_comment(int64_t commentId) -> void
{
    m_database->delete_comment(commentId);
    m_cachedComments.set_cache_expired({m_selectedProject.m_id, m_userSettingsData->m_loggedInUser.m_sessionToken});
    m_cachedActivity.set_cache_expired({m_selectedReview.m_reviewId, m_userSettingsData->m_loggedInUser.m_sessionToken});
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

    co_await concurrencpp::resume_on(get_app()->get_tick_executor());
    m_userFlowPopup.reset();
    m_cachedUsers.set_cache_expired(m_userSettingsData->m_loggedInUser.m_sessionToken);
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

auto workspace_manager::get_all_users() -> typename global_cache_type<user_data>::cache_result
{
    const gluten::token_cache_key key(m_userSettingsData->m_loggedInUser.m_sessionToken);

    if (m_cachedUsers.get_cache_needs_filling(key))
    {
        m_cachedUsers.set_async_fill_cache(key, transform_database_result_to_cache_result(m_database->get_all_users(key.m_token)));
        m_selectedUser = user_data();
    }

    return m_cachedUsers.get_cached_data(key);
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

    const auto& cachedUsers = m_cachedUsers.get_cached_data(gluten::token_cache_key(m_userSettingsData->m_loggedInUser.m_sessionToken));

    if (cachedUsers.m_state == gluten::cache_state::has_data)
    {
        for (const auto& user : cachedUsers.m_cache)
        {
            if (user.m_email == email)
            {
                m_selectedUser = user;
                break;
            }
        }
    }
}

auto workspace_manager::delete_user(const std::string& email) -> concurrencpp::result<void>
{
    if (!email.empty())
    {
        m_selectedUser = user_data();

        const gluten::token_cache_key key(m_userSettingsData->m_loggedInUser.m_sessionToken);

        m_cachedUsers.get_raw_data(key).erase(
            std::find_if(m_cachedUsers.get_raw_data(key).begin(), m_cachedUsers.get_raw_data(key).end(),
            [email](const user_data& user) -> bool 
            {
                return email == user.m_email;
            }));

        const bool deletingSelf = email == m_userSettingsData->m_loggedInUser.m_email;

        const tl::expected<bool, database_error> result = co_await m_database->delete_user(email, m_userSettingsData->m_loggedInUser.m_sessionToken);

        if (result.has_value())
        {
            if (result.value())
            {
                co_await concurrencpp::resume_on(review_app::get()->get_tick_executor());

                m_selectedProject = project_data();
                m_selectedReview  = review_data();

                m_cachedUsers.set_cache_expired(key);

                if (deletingSelf)
                {
                    logout();
                }
            }
        }
    }
}

auto workspace_manager::get_users_for_review(int64_t reviewId) -> typename default_cache_type<reviewer_data>::cache_result
{
    const gluten::key_and_token_cache_key key(reviewId, m_userSettingsData->m_loggedInUser.m_sessionToken);

    if (m_reviewUsersCache.get_cache_needs_filling(key))
    {
        m_reviewUsersCache.set_async_fill_cache(key, async_get_users_for_review(reviewId));
    }

    return m_reviewUsersCache.get_cached_data(key);
}

auto workspace_manager::set_users_for_review(int64_t reviewId, std::vector<int64_t> userIds) -> concurrencpp::result<tl::expected<bool, database_error>>
{
    const gluten::key_and_token_cache_key key(reviewId, m_userSettingsData->m_loggedInUser.m_sessionToken);
    m_reviewUsersCache.set_cache_expired(key);

    co_return co_await m_database->set_users_for_review(reviewId, userIds, m_userSettingsData->m_loggedInUser.m_sessionToken);
}

auto workspace_manager::set_user_vote_for_review(int64_t reviewId, int64_t userId, review_vote vote) -> concurrencpp::result<void>
{
    gluten::data_source<user_settings_data> userSettings;

    auto& rawReviewUsers = m_reviewUsersCache.get_raw_data({reviewId, userSettings->m_loggedInUser.m_sessionToken});

    auto foundReviewer = std::find_if(rawReviewUsers.begin(), rawReviewUsers.end(), [userId](const reviewer_data& reviewer) 
        {
            return reviewer.m_userId == userId;
        });

    if (foundReviewer != rawReviewUsers.end())
    {
        foundReviewer->m_vote = vote;
    }

    co_await m_database->set_user_vote_for_review(reviewId, userId, vote, userSettings->m_loggedInUser.m_sessionToken);
}