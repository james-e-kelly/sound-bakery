#pragma once

#include "pch.h"

class review_client : public gluten::manager
{
public:
    review_client(gluten::app* app, const std::filesystem::path& workspacePath);
    ~review_client() = default;

    auto exit() -> void override;

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

    std::unique_ptr<httplib::SSLClient> m_client;
};