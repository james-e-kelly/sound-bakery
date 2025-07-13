#pragma once

#include "pch.h"
#include "SQLiteCpp/SQLiteCpp.h"

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
    auto create_workspace(const std::string name) -> concurrencpp::result<void>;
    auto open_workspace(const std::string name) -> concurrencpp::result<void>;
    auto get_workspace_name() const -> concurrencpp::result<std::string>;

    auto create_project(const std::string name, const std::string description) -> concurrencpp::result<project_data>;
    auto get_all_projects() const -> concurrencpp::result<std::vector<project_data>>;

    auto create_review(int64_t projectId, const new_review_data newReview) -> concurrencpp::result<review_data>;
    auto update_review(const review_data review) -> concurrencpp::result<review_data>;
    auto get_all_reviews(int64_t projectId) const -> concurrencpp::result<std::vector<review_data>>;

private:
    SQLite::Database m_database;
};