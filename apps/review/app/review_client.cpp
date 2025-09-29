#include "review_client.h"

namespace
{
    template <typename T, typename... Args>
    auto get_api(const std::unique_ptr<httplib::SSLClient>& client, const std::string endpoint, httplib::Params params = httplib::Params()) -> concurrencpp::result<T>
    {
        co_await concurrencpp::resume_on(gluten::app::get()->background_executor());

        if (auto result = client->Get(endpoint, params, httplib::Headers()))
        {
            co_return review_app_serialization::deserialize_from_xml<T>(result.value().body);
        }

        co_return T{};
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
    co_return co_await get_api<std::vector<review_data>>(m_client, review_app_endpoints::reviews,
        httplib::Params{{review_app_parameters::projectId, std::to_string(projectId)}});
}

auto review_client::get_review_vote(database_id reviewId, database_id userId) -> concurrencpp::result<review_vote>
{
    co_return co_await get_api<review_vote>(m_client, review_app_api::getReviewVote,
        httplib::Params
        {
            {review_app_parameters::reviewId, std::to_string(reviewId)},
            {review_app_parameters::userId, std::to_string(userId)}
        });
}

auto review_client::user_table_is_empty() -> concurrencpp::result<bool>
{
    co_return co_await get_api<bool>(m_client, review_app_api::userTableIsEmpty);
}

auto review_client::post_project(const std::string projectName, const std::string projectDescription) -> concurrencpp::result<database_id>
{
    co_await concurrencpp::resume_on(gluten::app::get()->background_executor());

    const auto postResult = m_client->Post(review_app_endpoints::projects, httplib::Headers(), httplib::Params 
        {
            {
                review_app_parameters::name, 
                projectName
            },
            {
                review_app_parameters::description, 
                projectDescription
            }
        });
    
    if (postResult)
    {
        
    }

    co_return 0;
}

auto review_client::post_review(database_id projectId, const new_transit_review_data newReview) -> concurrencpp::result<tl::expected<review_data,bool>>
{
    httplib::UploadFormDataItems items = {
        {
            review_app_parameters::projectId,
            std::to_string(projectId),
            "",
            "text/plain"
        }};

    items.push_back(
        {
            review_app_parameters::data, 
            review_app_serialization::serialize_to_xml<new_transit_review_data>(newReview),
            "",
            "application/xml"
        });

    for (const auto contextFile : newReview.m_contextFiles)
    {
        items.push_back(
            {
                review_app_parameters::contextFile,
                std::string(reinterpret_cast<const char*>(contextFile.m_fileData.data()), contextFile.m_fileData.size()),
                contextFile.m_fileName,
                "application/octet-stream"
            });
    }

    for (const auto reviewFile : newReview.m_reviewFiles)
    {
        items.push_back(
            {
                review_app_parameters::reviewFile,
                std::string(reinterpret_cast<const char*>(reviewFile.m_fileData.data()), reviewFile.m_fileData.size()),
                reviewFile.m_fileName,
                "application/octet-stream"
            });
    }
    
    review_data review;

    co_await concurrencpp::resume_on(gluten::app::get()->background_executor());

    const httplib::Result postResult = m_client->Post(review_app_endpoints::reviews, httplib::Headers(), items);

    if (http_result_okay(postResult))
    {
        review = review_app_serialization::deserialize_from_xml<review_data>(postResult.value().body);
    }

    co_return review;
}