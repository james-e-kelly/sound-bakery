#include "review_client.h"

review_client::review_client(gluten::app* app, const std::filesystem::path& workspacePath)
    : gluten::manager(app)
{
    std::error_code errorCode;
    if (std::filesystem::exists(workspacePath, errorCode))
    {
        m_client = std::make_unique<httplib::SSLClient>("localhost", 8080);
        m_client->enable_server_certificate_verification(false);

        httplib::Result result = m_client->Get("/ping");
        if (result) 
        {
            const std::string value = result.value().body;
        }
    }
}

auto review_client::exit() -> void
{
    if (m_client)
    {
        m_client->stop();
    }
}