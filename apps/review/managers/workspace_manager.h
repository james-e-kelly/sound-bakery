#pragma once

#include "pch.h"

#include "app/review_database.h"
#include "data/activity_data.h"
#include "data/comment_data.h"
#include "data/user_data.h"
#include "data/review_data.h"
#include "data/workspace_data.h"
#include "data/project_data.h"
#include "data/user_settings_data.h"

class intro_widget;
class review_database;
class user_flow_popup;
class workspace_widget;


template <typename data_type, typename key_type = int64_t>
struct cache
{
    [[nodiscard]] auto contains(const key_type& key) const -> bool { return m_cache.contains(key); }
    [[nodiscard]] auto cache_count_valid(const key_type& key, const std::size_t& currentDataSize) const -> bool
    {
        bool result = false;

        if (contains(key))
        {
            if (m_cache.at(key).size() == currentDataSize)
            {
                result = true;
            }
        }

        return result;
    }
    [[nodiscard]] auto get_cache(const key_type& key) const -> const std::vector<data_type>&
    {
        if (contains(key))
        {
            return m_cache.at(key);
        }
        static std::vector<data_type> invalid;
        return invalid;
    }

    auto erase_cache(const key_type& key) -> void { m_cache.erase(key); }
    auto add_cache(const key_type& key, const std::vector<data_type>& data) -> void
    {
        m_cache.insert({key, std::move(data)});
    }

    std::unordered_map<key_type, std::vector<data_type>> m_cache;
};

class workspace_manager : public gluten::manager	
{
public:
    workspace_manager(gluten::app* app) : gluten::manager(app) {}
    ~workspace_manager();

    auto open_workspace(const std::filesystem::path& workspaceFile) -> void;
    auto create_workspace(const std::string& workspaceName, const std::filesystem::path& workspaceDirectory) -> void;
    auto close_workspace() -> void;

    auto add_existing_project(const project_data& projectData) -> void;
    auto create_project(const std::string& projectName, const std::string& projectDescription) -> void;
    auto select_project(const std::string& projectName) -> void;
    [[nodiscard]] auto has_selected_project() const -> bool;

    [[nodiscard]] auto get_selected_project() const -> const project_data&;
    [[nodiscard]] auto get_projects() const -> const std::set<project_data>&;
    
    [[nodiscard]] auto get_workspace_name() const -> std::string;
    [[nodiscard]] auto get_workspace_file() const -> std::filesystem::path;
    [[nodiscard]] auto get_workspace_directory() const -> std::filesystem::path;

    auto select_review(int64_t reviewId) -> void;
    auto get_selected_review() const -> const review_data&;
    auto create_review(const new_review_data& newReview) -> void;
    auto update_review(const review_data& updatedReview) -> void;
    auto get_all_reviews() const -> const std::set<review_data>&;

    // Activity
    auto get_all_activity_for_review(int64_t reviewId) const -> const std::vector<activity_data>&;
    auto get_activity_count_for_review(int64_t reviewId) const -> std::size_t;

    // Comments
    auto get_all_comments_for_review(int64_t reviewId) const -> const std::vector<comment_data>&;
    auto get_comments_count_for_review(int64_t reviewId) const -> std::size_t;
    auto create_comment(const new_comment_data& newComment) -> void;
    auto delete_comment(int64_t commentId) -> void;

    // Users
    auto open_create_user_popup() -> void;
    auto users_table_is_empty() -> concurrencpp::result<bool>;
    auto logged_in_user_can_create_users() -> concurrencpp::result<bool>;
    auto create_user(const new_user_data newUser, std::optional<std::string> userToken) -> concurrencpp::result<tl::expected<bool, database_error>>;
    auto create_user_and_login(new_user_data newUser) -> concurrencpp::result<tl::expected<bool, database_error>>;
    auto get_all_users() -> const std::vector<user_data>&;
    auto get_users_count() const -> std::size_t;
    auto get_selected_user() const -> const user_data&;
    auto select_user(const std::string& email) -> void;
    auto delete_user(const std::string& email) -> void;

    // Login / Logout
    auto login_user(login_request_data loginData) -> concurrencpp::result<tl::expected<bool, database_error>>;
    auto logout() -> void;

    // Review Users
    auto get_users_for_review(int64_t reviewId) -> const std::vector<reviewer_data>&;
    auto set_users_for_review(int64_t reviewId, std::vector<int64_t> userIds) -> concurrencpp::result<tl::expected<bool, database_error>>;

protected:
    auto init(gluten::app* app) -> void override;
    auto start() -> void override;

private:
    static auto file_is_workspace(const std::filesystem::path& file) -> bool;

    auto open_workspace_widget() -> void;
    auto open_user_flow_popup() -> void;

    auto load_projects_from_workspace() -> void;

    std::shared_ptr<intro_widget> m_introWidget;
    std::shared_ptr<workspace_widget> m_workspaceWidget;
    std::shared_ptr<user_flow_popup> m_userFlowPopup;

    gluten::data_source<user_settings_data> m_userSettingsData;
    std::set<project_data> m_projects;
    std::set<review_data> m_reviews;
    project_data m_selectedProject;
    review_data m_selectedReview;
    user_data m_selectedUser;
    std::shared_ptr<review_database> m_database;

    cache<user_data> m_allUsersCache;
    cache<reviewer_data> m_reviewUsersCache;
    cache<std::unordered_map<int64_t, review_vote>> m_reviewVotesCache;
};