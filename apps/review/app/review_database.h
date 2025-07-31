#pragma once

#include "pch.h"
#include "SQLiteCpp/SQLiteCpp.h"

#include "data/activity_data.h"
#include "data/comment_data.h"
#include "data/user_data.h"
#include "data/review_data.h"
#include "data/project_data.h"

enum class database_error_code
{
    error,              //< Generic error
    missing_table,      //< The table does not exist
    no_data,            //< Could not get a row or some other piece of data
    unauthorized,       //< No permission / not allowed
    invalid_parameters  //< User supplied incorrect parameters
};

struct database_error
{
    database_error_code m_errorCode = database_error_code::error;
    std::string m_errorMessage;
};

/**
 * @brief Handles database creation, deletion, updates and so on.
 */
class review_database
{
public:
    review_database() = delete;
    review_database(const std::filesystem::path& databasePath);
    ~review_database() = default;

    using bool_result = concurrencpp::result<tl::expected<bool, database_error>>;
    using user_id = int64_t;

public:
    // Workspace
    auto create_workspace(const std::string name) -> concurrencpp::result<void>;
    auto open_workspace(const std::string name) -> concurrencpp::result<void>;
    auto get_workspace_name() const -> concurrencpp::result<std::string>;

    // Project
    auto create_project(const std::string name, const std::string description) -> concurrencpp::result<project_data>;
    auto get_all_projects() const -> concurrencpp::result<std::vector<project_data>>;

    // Review
    auto create_review(int64_t projectId, const new_review_data newReview) -> concurrencpp::result<review_data>;
    auto update_review(const review_data review) -> concurrencpp::result<review_data>;
    auto get_all_reviews(int64_t projectId) const -> concurrencpp::result<std::vector<review_data>>;
    auto get_user_vote_on_review(int64_t reviewId, int64_t userId) const -> concurrencpp::result<tl::expected<review_vote, database_error>>;

    // Activity
    auto get_all_activity_for_review(int64_t reviewId) const -> concurrencpp::result<std::vector<activity_data>>;
    auto get_activity_count_for_review(int64_t reviewId) const -> concurrencpp::result<std::size_t>;

    // Comments
    auto get_all_comments_for_review(int64_t reviewId) const -> concurrencpp::result<std::vector<comment_data>>;
    auto get_comments_count_for_review(int64_t commentId) const -> concurrencpp::result<std::size_t>;
    auto create_comment(new_comment_data newComment) -> concurrencpp::result<comment_data>;
    auto delete_comment(int64_t commentId) -> concurrencpp::result<void>;

    // Users
    auto create_user(new_user_data newUser, std::string userToken) -> concurrencpp::result<tl::expected<bool, database_error>>;
    auto user_table_is_empty() const -> concurrencpp::result<bool>; //< If the table is empty, we allow an admin account to be created
    auto user_is_logged_in_and_has_privilege_for_action(std::string userToken, activity_type activity) -> concurrencpp::result<bool>;
    auto user_is_logged_in_and_has_privilege(std::string userToken, user_privileges privilege) -> concurrencpp::result<bool>;
    auto login_user(login_request_data loginRequest) -> concurrencpp::result<tl::expected<logged_in_user_data, database_error>>;
    auto get_users_count(std::string userToken) -> concurrencpp::result<tl::expected<std::size_t, database_error>>;
    auto get_all_users(std::string userToken) -> concurrencpp::result<tl::expected<std::vector<user_data>, database_error>>;
    auto delete_user(std::string email, std::string userToken) -> concurrencpp::result<tl::expected<bool, database_error>>;

    // Review Users
    auto get_users_for_review(int64_t reviewId, std::string userToken) -> concurrencpp::result<tl::expected<std::vector<user_data>, database_error>>;
    auto set_users_for_review(int64_t reviewId, std::vector<int64_t> userIds, std::string userToken) -> bool_result;

private:
    SQLite::Database m_database;
};