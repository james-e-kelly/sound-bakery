#pragma once

#include "pch.h"

#include "data/project_data.h"
#include "data/review_data.h"
#include "data/user_settings_data.h"

class review_client : public gluten::manager
{
public:
    review_client(gluten::app* app, const std::filesystem::path& workspacePath);
    ~review_client() = default;

    auto exit() -> void override;

    auto get_workspace_name() -> concurrencpp::result<std::string>;
    auto get_all_projects() -> concurrencpp::result<std::vector<project_data>>;
    auto get_all_reviews(database_id projectId) -> concurrencpp::result<std::vector<review_data>>;
    auto get_review_vote(database_id reviewId, database_id userId) -> concurrencpp::result<review_vote>;

    auto user_table_is_empty() -> concurrencpp::result<bool>;

private:

    auto get_user_session_token() const -> std::string
    {
        return m_userSettingsData->m_loggedInUser.m_sessionToken;
    }

    gluten::data_source<user_settings_data> m_userSettingsData;
    std::unique_ptr<httplib::SSLClient> m_client;
};