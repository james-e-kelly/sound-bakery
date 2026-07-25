#pragma once

#include "pch.h"

#include "app/review_database.h"
#include "data/activity_data.h"
#include "data/comment_data.h"
#include "data/project_data.h"
#include "data/review_data.h"
#include "data/user_data.h"
#include "data/user_settings_data.h"
#include "data/workspace_data.h"

class review_client;
class review_database;
class user_flow_popup;
class workspace_widget;

class workspace_manager : public gluten::manager
{
public:
    template <typename data_type>
    using default_cache_type = gluten::data_cache<std::vector<data_type>, gluten::key_and_token_cache_key<int64_t, std::string>, gluten::key_and_token_cache_key_hasher<int64_t, std::string>>;

    // Cache type that doesn't need any lookup. The logged in user is the only key
    template <typename data_type>
    using global_cache_type = gluten::data_cache<std::vector<data_type>, gluten::token_cache_key<std::string>, gluten::token_cache_key_hasher<std::string>>;

    using string_cache_type = gluten::data_cache<std::string, gluten::token_cache_key<std::string>, gluten::token_cache_key_hasher<std::string>>;

    // Relative path key -> absolute path cache
    using file_cache_type = gluten::data_cache<std::filesystem::path, gluten::key_and_token_cache_key<std::filesystem::path, std::string>, gluten::key_and_token_cache_key_hasher<std::filesystem::path, std::string>>;

    workspace_manager(gluten::app* app) : gluten::manager(app) {}
    ~workspace_manager();

    // Workspace
    [[nodiscard]] auto get_workspace_name() -> typename string_cache_type::cache_result;
    [[nodiscard]] auto get_workspace_file() const -> std::filesystem::path;
    [[nodiscard]] auto get_workspace_directory() const -> std::filesystem::path;
    auto open_client(const std::shared_ptr<review_client> client) -> concurrencpp::result<void>;
    auto close_workspace() -> void;

    // Projects
    [[nodiscard]] auto get_selected_project() const -> const project_data&;
    [[nodiscard]] auto get_all_projects() -> typename global_cache_type<project_data>::cache_result;
    [[nodiscard]] auto has_selected_project() const -> bool;
    auto create_project(const std::string& projectName, const std::string& projectDescription) -> void;
    auto select_project(const std::string projectName) -> concurrencpp::result<void>;
    auto delete_project(const std::string& proejctName) -> concurrencpp::result<void>;
    auto get_project_users(database_id projectId) -> typename default_cache_type<user_data>::cache_result;  // Users assigned to this project. It is viewable to them and possibly editable if they have the user privileges
    auto set_project_users(database_id projectId, std::vector<user_data> users) -> concurrencpp::result<void>;

    // Reviews
    auto select_review(int64_t reviewId) -> void;
    auto get_selected_review() const -> const review_data&;
    auto create_review(const new_frontend_review_data newReview) -> concurrencpp::result<void>;
    auto update_review(const review_data updatedReview) -> concurrencpp::result<void>;
    auto get_all_reviews() -> typename default_cache_type<review_data>::cache_result;
    auto delete_review(int64_t reviewId) -> concurrencpp::result<void>;
    auto create_review_version(int64_t reviewId, new_frontend_review_data newReviewVersion) -> concurrencpp::result<void>;
    auto set_review_status(database_id reviewId, review_status status) -> concurrencpp::result<void>;

    // Activity
    auto get_all_review_activity(int64_t reviewId) -> typename default_cache_type<activity_data>::cache_result;

    // Comments
    auto get_all_comments_for_review(int64_t reviewId) -> typename default_cache_type<comment_data>::cache_result;
    auto create_comment(const new_comment_data newComment) -> concurrencpp::result<void>;
    auto delete_comment(int64_t commentId) -> concurrencpp::result<void>;

    // Users
    auto open_create_user_popup() -> void;
    auto create_user(const new_user_data newUser, std::optional<std::string> userToken) -> concurrencpp::result<tl::expected<bool, database_error>>;
    auto create_user_and_login(new_user_data newUser) -> concurrencpp::result<tl::expected<bool, database_error>>;
    auto get_all_users() -> typename global_cache_type<user_data>::cache_result;
    auto get_selected_user() const -> const user_data&;
    auto select_user(const std::string& email) -> void;
    auto delete_user(database_id userId) -> concurrencpp::result<void>;
    auto get_user(int64_t userId) -> user_data;

    // Login / Logout
    auto login_user(login_request_data loginData) -> concurrencpp::result<tl::expected<bool, database_error>>;
    auto logout() -> void;

    // Review Users
    auto get_review_users(int64_t reviewId) -> typename default_cache_type<reviewer_data>::cache_result;
    auto set_review_users(int64_t reviewId, std::vector<int64_t> userIds) -> concurrencpp::result<void>;

    // Voting
    auto set_review_vote(int64_t reviewId, int64_t userId, review_vote vote) -> concurrencpp::result<void>;

    auto get_user_session_token() const -> std::string;
    auto get_user_session_has_expired() const -> bool;
    auto get_user_privileges() const -> user_privileges;

    auto get_review_file(const std::filesystem::path& relativeFilePath) -> typename file_cache_type::cache_result;

    auto stop_all_files() const -> void;

protected:
    auto init(gluten::app* app) -> void override;
    auto start() -> void override;

private:
    static auto file_is_workspace(const std::filesystem::path& file) -> bool;

    auto async_get_users_for_review(int64_t reviewId) -> concurrencpp::result<std::vector<reviewer_data>>;
    auto async_get_review_file(std::filesystem::path relativeFilePath) -> concurrencpp::result<file_cache_type::cache_data_type>;

    auto open_workspace_widget() -> void;
    auto open_user_flow_popup() -> concurrencpp::result<void>;

    std::shared_ptr<workspace_widget> m_workspaceWidget;
    std::shared_ptr<user_flow_popup> m_userFlowPopup;

    gluten::data_source<user_settings_data> m_userSettingsData;
    project_data m_selectedProject;
    review_data m_selectedReview;
    user_data m_selectedUser;
    std::shared_ptr<class review_server> m_server;
    std::shared_ptr<class review_client> m_client;

    // Caches
    default_cache_type<comment_data> m_cachedComments;
    global_cache_type<user_data> m_cachedUsers;
    default_cache_type<user_data> m_projectUsersCache;
    default_cache_type<reviewer_data> m_reviewUsersCache;
    default_cache_type<activity_data> m_cachedActivity;
    default_cache_type<review_data> m_cachedReviews;
    global_cache_type<project_data> m_cachedProjects;
    string_cache_type m_cachedWorkspaceName;
    file_cache_type m_filesCache;
};