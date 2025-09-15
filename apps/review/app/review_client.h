#pragma once

#include "pch.h"

#include "data/user_settings_data.h"

class review_client : public gluten::manager
{
public:
    review_client(gluten::app* app, const std::filesystem::path& workspacePath);
    ~review_client() = default;

    auto exit() -> void override;

    auto get_workspace_name() -> concurrencpp::result<std::string>;

private:
    static auto ping(const httplib::Request& request,
                     httplib::Response& response) -> void
    {
        response.set_content("pong", "text/plain");
    }

    static auto get_workspace_name(const httplib::Request& request,
                                   httplib::Response& response) -> void
    {
        response.set_content("Hello World!", "text/plain");
    }

    auto get_user_session_token() const -> std::string
    {
        return m_userSettingsData->m_loggedInUser.m_sessionToken;
    }

    gluten::data_source<user_settings_data> m_userSettingsData;
    std::unique_ptr<httplib::SSLClient> m_client;
};