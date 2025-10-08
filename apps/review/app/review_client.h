#pragma once

#include "pch.h"

#include "data/activity_data.h"
#include "data/comment_data.h"
#include "data/project_data.h"
#include "data/review_data.h"
#include "data/user_settings_data.h"

class review_client : public gluten::manager
{
public:
    review_client(gluten::app* app, const std::string& serverAddress);
    ~review_client() = default;

    auto exit() -> void override;

    auto get_workspace_name() -> concurrencpp::result<std::string>;
    auto get_user_can_perform_action(activity_type activityType) -> concurrencpp::result<bool>;
    auto login(login_request_data loginRequestData) -> concurrencpp::result<logged_in_user_data>;

    auto get_all_projects() -> concurrencpp::result<std::vector<project_data>>;
    auto get_all_reviews(database_id projectId) -> concurrencpp::result<std::vector<review_data>>;
    auto get_all_comments_for_review(database_id reviewId) -> concurrencpp::result<std::vector<comment_data>>;
    auto get_review_vote(database_id reviewId, database_id userId) -> concurrencpp::result<review_vote>;
    auto get_all_users(database_id userId, database_id reviewId) -> concurrencpp::result<std::vector<user_data>>;
    auto get_all_review_activity(database_id reviewId) -> concurrencpp::result<std::vector<activity_data>>;

    auto user_is_logged_in() -> concurrencpp::result<bool>;
    auto user_table_is_empty() -> concurrencpp::result<bool>;

    auto post_project(const std::string projectName, const std::string projectDescription) -> concurrencpp::result<database_id>;
    auto post_review(database_id projectId, const new_transit_review_data newReview) -> concurrencpp::result<tl::expected<review_data,bool>>;
    auto post_review_version(database_id reviewId, new_transit_review_data newReviewData) -> concurrencpp::result<tl::expected<review_data, bool>>;
    auto post_comment(new_comment_data comment) -> concurrencpp::result<comment_data>;
    auto post_user(new_user_data newUser) -> concurrencpp::result<user_data>;

    auto put_review_status(database_id reviewId, review_status status) -> concurrencpp::result<void>;
    auto put_review_vote(database_id reviewId, database_id userId, review_vote vote) -> concurrencpp::result<void>;
    auto put_review(review_data reviewData) -> concurrencpp::result<void>;
    auto put_review_users(database_id reviewId, std::vector<database_id> userIds) -> concurrencpp::result<void>;

    auto delete_review(database_id reviewId) -> concurrencpp::result<void>;
    auto delete_comment(database_id commentId) -> concurrencpp::result<void>;
    auto delete_user(database_id userId) -> concurrencpp::result<void>;

private:
    auto get_user_session_token() const -> std::string
    {
        return m_userSettingsData->m_loggedInUser.m_sessionToken;
    }

    gluten::data_source<user_settings_data> m_userSettingsData;
    std::unique_ptr<httplib::SSLClient> m_client;
};