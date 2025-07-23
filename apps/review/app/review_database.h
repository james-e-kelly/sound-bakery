#pragma once

#include "pch.h"
#include "SQLiteCpp/SQLiteCpp.h"

#include "data/activity_data.h"
#include "data/comment_data.h"
#include "data/review_data.h"
#include "data/project_data.h"

/**
 * @brief Handles database creation, deletion, updates and so on.
 */
class review_database
{
public:
    review_database() = delete;
    review_database(const std::filesystem::path& databasePath);
    ~review_database() = default;

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

    // Activity
    auto get_all_activity_for_review(int64_t reviewId) const -> concurrencpp::result<std::vector<activity_data>>;
    auto get_activity_count_for_review(int64_t reviewId) const -> concurrencpp::result<std::size_t>;

    // Comments
    auto get_all_comments_for_review(int64_t reviewId) const -> concurrencpp::result<std::vector<comment_data>>;
    auto get_comments_count_for_review(int64_t commentId) const -> concurrencpp::result<std::size_t>;

private:
    SQLite::Database m_database;
};