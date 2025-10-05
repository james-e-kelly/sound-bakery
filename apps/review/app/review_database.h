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

/**
 * @brief Describes a database error through an error code and error message.
 */
struct database_error
{
    database_error_code m_errorCode = database_error_code::error;
    std::string m_errorMessage;
};

/**
 * @brief Handles database creation, deletion, updates, queries and more.
 */
class review_database : public std::enable_shared_from_this<review_database>
{
public:
    review_database() = delete;
    review_database(const std::filesystem::path& databasePath);
    ~review_database() = default;

    template<typename T>
    using database_result = concurrencpp::result<tl::expected<T, database_error>>;

    using bool_result = database_result<bool>;

public:
    auto create_workspace(const std::string name) const                                                                             -> bool_result;
    auto open_workspace(const std::string name) const                                                                               -> bool_result;
    auto login_user(login_request_data loginRequest) const                                                                          -> database_result<logged_in_user_data>;
    
    auto create_user(new_user_data newUser, std::string userToken) const                                                            -> database_result<user_data>;
    auto create_project(const std::string name, const std::string description, std::string userToken) const                         -> database_result<project_data>;
    auto create_review(database_id projectId, const new_transit_review_data newReview, std::string userToken) const                 -> database_result<review_data>;
    auto create_review_version(database_id reviewId, const new_transit_review_data newReviewVersion, std::string userToken) const   -> database_result<review_data>;
    auto create_comment(new_comment_data newComment, std::string userToken) const                                                   -> database_result<comment_data>;
    
    auto get_workspace(std::string userToken) const                                                                                 -> database_result<workspace_data>;
    auto get_all_projects(std::string userToken) const                                                                              -> database_result<std::vector<project_data>>;
    auto get_all_reviews(database_id projectId, std::string userToken) const                                                        -> database_result<std::vector<review_data>>;
    auto get_review_votes(database_id reviewId, database_id userId, std::string userToken) const                                    -> database_result<std::vector<review_vote>>;
    auto get_all_review_activity(database_id reviewId, std::string userToken) const                                                 -> database_result<std::vector<activity_data>>;

    /**
     * @brief Gets all comments in the database, optionally filtered.
     * @param reviewId optional review filter. Values of 0 or less returns comments from all reviews.
     * @param commentId optional review filter. Values of 0 or less returns all comments on the review or database, depending on the value of @see reviewId.
     * @param userToken to check authority with.
     * @return A vector of the comments found. Can be 0, 1, or more.
     */
    auto get_all_comments(database_id reviewId, database_id commentId, std::string userToken) const                                 -> database_result<std::vector<comment_data>>;

    /**
     * @brief Get all users in the database, optionally filtered.
     * @param userId optional filter to return just the user data with the passed ID.
     * @param userToken userToken to check authority with.
     * @return A vector of all users or a vector with the single user in. Can also empty.
     */
    auto get_all_users(database_id userId, std::string userToken) const                                                             -> database_result<std::vector<user_data>>;
    auto get_review_users(database_id reviewId, std::string userToken) const                                                        -> database_result<std::vector<user_data>>;

    auto update_review(const review_data review, std::string userToken) const                                                       -> database_result<review_data>;

    auto set_review_users(database_id reviewId, std::vector<database_id> userIds, std::string userToken) const                      -> bool_result;
    auto set_review_vote(database_id reviewId, database_id userId, review_vote vote, std::string userToken) const                   -> bool_result;
    auto set_review_status(database_id reviewId, review_status status, std::string userToken) const                                 -> bool_result;

    auto delete_review(database_id reviewId, std::string userToken) const                                                           -> bool_result;
    auto delete_comment(database_id commentId, std::string userToken) const                                                         -> bool_result;
    auto delete_user(std::string email, std::string userToken) const                                                                -> bool_result;

    auto user_can_perform_action(std::string userToken, activity_type activity) const                                               -> bool_result;
    auto user_has_privilege(std::string userToken, user_privileges privilege) const                                                 -> bool_result;
private:
    SQLite::Database m_database;
};