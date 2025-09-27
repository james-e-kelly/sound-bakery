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

template <typename T>
T deserialize_from_xml(const std::string& body)
{
    T data;
    std::istringstream inputStream(body);
    boost::archive::xml_iarchive archive(inputStream);
    archive & BOOST_SERIALIZATION_NVP(data);
    return data;
}

auto review_client::get_workspace_name() -> concurrencpp::result<std::string>
{
    co_await concurrencpp::resume_on(get_app()->background_executor());

    if (httplib::Result result = m_client->Get(review_app_api::getworkspaceName))
    {
        co_return deserialize_from_xml<std::string>(result.value().body);
    }
    
    co_return std::string();
}

auto review_client::get_all_projects() -> concurrencpp::result<std::vector<project_data>>
{
    co_await concurrencpp::resume_on(get_app()->background_executor());
    
    if (httplib::Result result = m_client->Get(review_app_api::getAllProjects))
    {
        co_return deserialize_from_xml<std::vector<project_data>>(result.value().body);
    }

    co_return std::vector<project_data>();
}

auto review_client::get_all_reviews(database_id projectId) -> concurrencpp::result<std::vector<review_data>>
{
    co_await concurrencpp::resume_on(get_app()->background_executor());

    httplib::Params params;
    params.emplace(review_app_parameters::projectId, std::to_string(projectId));

    if (httplib::Result result = m_client->Get(review_app_api::getAllReviews, params, httplib::Headers()))
    {
        co_return deserialize_from_xml<std::vector<review_data>>(result.value().body);
    }

    co_return std::vector<review_data>();
}

auto review_client::get_review_vote(database_id reviewId, database_id userId) -> concurrencpp::result<review_vote>
{
    co_await concurrencpp::resume_on(get_app()->background_executor());

    httplib::Params params;
    params.emplace(review_app_parameters::reviewId, std::to_string(reviewId));
    params.emplace(review_app_parameters::userId, std::to_string(userId));

    if (httplib::Result result = m_client->Get(review_app_api::getReviewVote, params, httplib::Headers()))
    {
        co_return deserialize_from_xml<review_vote>(result.value().body);
    }

    co_return review_vote();
}