#pragma once

#include "pch.h"

#include "app/review_database.h"

class review_server : public gluten::manager
{
public:
    review_server(gluten::app* app, const std::filesystem::path& workspacePath);
    ~review_server() = default;

    auto start() -> void override;
    auto exit() -> void override;

private:
    static auto ping(const httplib::Request& request, httplib::Response& response) -> void
    {
        response.set_content("pong", "text/plain");
    }

    static auto get_workspace_name(const httplib::Request& request,
                                   httplib::Response& response) -> void;

    std::unique_ptr<httplib::SSLServer> m_server;
};