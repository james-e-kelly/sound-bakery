#include "review_client.h"

review_client::review_client(gluten::app* app, const std::filesystem::path& workspacePath)
    : gluten::manager(app)
{
    std::error_code errorCode;
    if (std::filesystem::exists(workspacePath, errorCode))
    {
        m_client = std::make_unique<httplib::SSLClient>("localhost", 8080);
        m_client->enable_server_certificate_verification(false);

        m_client->set_bearer_token_auth(get_user_session_token());

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

auto review_client::get_workspace_name() -> concurrencpp::result<std::string>
{
    BOOST_ASSERT(m_client);

    co_await concurrencpp::resume_on(get_app()->background_executor());

    httplib::Result result = m_client->Get("/get_workspace_name");
    if (result)
    {
        co_return result.value().body;
    }
    co_return std::string();
}