#include "review_client.h"

namespace
{
    template <typename T>
    T deserialize_from_xml(const std::string& body)
    {
        T data;
        std::istringstream inputStream(body);
        boost::archive::xml_iarchive archive(inputStream);
        archive& BOOST_SERIALIZATION_NVP(data);
        return data;
    }

    template <typename T, typename... Args>
    auto get_api(const std::unique_ptr<httplib::SSLClient>& client, const std::string& endpoint, Args&&... args) -> concurrencpp::result<T>
    {
        co_await concurrencpp::resume_on(gluten::app::get()->background_executor());

        httplib::Params params;
        ((params.emplace(std::forward<Args>(args).first,
                         std::forward<Args>(args).second)),
         ...);

        if (auto result = client->Get(endpoint, params, httplib::Headers()))
        {
            co_return deserialize_from_xml<T>(result.value().body);
        }

        co_return T{};
    }

    template <typename... Args>
    auto post_api(const std::unique_ptr<httplib::SSLClient>& client, const std::string& endpoint, Args&&... args) -> concurrencpp::result<void>
    {
        co_await concurrencpp::resume_on(gluten::app::get()->background_executor());

        httplib::Params params;
        ((params.emplace(std::forward<Args>(args).first,
                         std::forward<Args>(args).second)),
         ...);

        const httplib::Result result = client->Post(endpoint, httplib::Headers(), params);
    }
}

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
    co_return co_await get_api<std::string>(m_client, review_app_api::getWorkspaceName);
}

auto review_client::get_all_projects() -> concurrencpp::result<std::vector<project_data>>
{
    co_return co_await get_api<std::vector<project_data>>(m_client, review_app_endpoints::projects);
}

auto review_client::get_all_reviews(database_id projectId) -> concurrencpp::result<std::vector<review_data>>
{
    co_return co_await get_api<std::vector<review_data>>(m_client, review_app_api::getAllReviews,
        std::make_pair(review_app_parameters::projectId, std::to_string(projectId)));
}

auto review_client::get_review_vote(database_id reviewId, database_id userId) -> concurrencpp::result<review_vote>
{
    co_return co_await get_api<review_vote>(m_client, review_app_api::getReviewVote,
                         std::make_pair(review_app_parameters::reviewId, std::to_string(reviewId)),
                         std::make_pair(review_app_parameters::userId, std::to_string(userId)));
}

auto review_client::user_table_is_empty() -> concurrencpp::result<bool>
{
    co_return co_await get_api<bool>(m_client, review_app_api::userTableIsEmpty);
}

auto review_client::post_project(const std::string projectName, const std::string projectDescription) -> concurrencpp::result<void>
{
    co_return co_await post_api(
        m_client, review_app_endpoints::projects,
        std::make_pair(review_app_parameters::name, projectName),
        std::make_pair(review_app_parameters::description, projectDescription));
}