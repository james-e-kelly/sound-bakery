#include "review_client.h"

namespace
{
    template <typename T>
    auto get_api(const std::unique_ptr<httplib::SSLClient>& client, const std::string endpoint, httplib::Params params = httplib::Params()) -> concurrencpp::result<T>
    {
        co_await concurrencpp::resume_on(gluten::app::get()->background_executor());

        if (auto result = client->Get(endpoint, params, httplib::Headers()))
        {
            if (http_result_okay(result))
            {
                co_return review_app_serialization::deserialize_from_xml<T>(result.value().body);
            }
        }

        co_return T{};
    }

    template <typename T>
    auto post_form_api(const std::unique_ptr<httplib::SSLClient>& client, const std::string endpoint, httplib::UploadFormDataItems items) -> concurrencpp::result<T>
    {
        co_await concurrencpp::resume_on(gluten::app::get()->background_executor());

        const httplib::Result postResult = client->Post(endpoint, httplib::Headers(), items);

        T result;

        if (http_result_okay(postResult))
        {
            result = review_app_serialization::deserialize_from_xml<T>(postResult.value().body);
        }

        co_return result;
    }

    auto put_api(const std::unique_ptr<httplib::SSLClient>& client, const std::string endpoint, httplib::Params params = httplib::Params()) -> concurrencpp::result<void>
    {
        co_await concurrencpp::resume_on(gluten::app::get()->background_executor());

        client->Put(endpoint, params);

        co_return;
    }

    auto put_form_api(const std::unique_ptr<httplib::SSLClient>& client, const std::string endpoint, httplib::UploadFormDataItems items) -> concurrencpp::result<void>
    {
        co_await concurrencpp::resume_on(gluten::app::get()->background_executor());

        const httplib::Result postResult = client->Put(endpoint, httplib::Headers(), items);

        co_return;
    }

    auto delete_api(const std::unique_ptr<httplib::SSLClient>& client, const std::string endpoint, httplib::Params params = httplib::Params()) -> concurrencpp::result<void>
    {
        co_await concurrencpp::resume_on(gluten::app::get()->background_executor());

        auto result = client->Delete(endpoint, params);

        co_return;
    }
}

review_client::review_client(gluten::app* app, const std::string& serverAddress)
    : gluten::manager(app)
{
    if (!serverAddress.empty())
    {
        m_client = std::make_unique<httplib::SSLClient>(serverAddress, 8080);

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
    co_return (co_await get_api<workspace_data>(m_client, review_app_endpoints::workspace)).m_workspaceName;
}

auto review_client::get_user_can_perform_action(activity_type activityType) -> concurrencpp::result<bool>
{
    co_return co_await get_api<bool>(m_client, review_app_endpoints::me, httplib::Params
        {
            { review_app_parameters::activityType, std::to_string((int)activityType) }
        });
}

auto review_client::login(login_request_data loginRequestData) -> concurrencpp::result<logged_in_user_data>
{
    httplib::UploadFormDataItems items = 
    {
        {
            review_app_parameters::data,
            review_app_serialization::serialize_to_xml<login_request_data>(loginRequestData),
            "", "application/xml"
        }
    };

    const logged_in_user_data result = co_await post_form_api<logged_in_user_data>(m_client, review_app_endpoints::login, items);

    m_client->set_bearer_token_auth(result.m_sessionToken);

    co_return result;
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

auto review_client::get_all_comments_for_review(database_id reviewId) -> concurrencpp::result<std::vector<comment_data>>
{
    co_return co_await get_api<std::vector<comment_data>>(m_client, review_app_endpoints::comments, httplib::Params
        {
            { review_app_parameters::reviewId, std::to_string(reviewId) }
        });
}

auto review_client::get_review_vote(database_id reviewId, database_id userId) -> concurrencpp::result<review_vote>
{
    const auto reviewVotes = co_await get_api<std::vector<review_vote>>(m_client, review_app_endpoints::reviewVotes,
        httplib::Params
        {
            {review_app_parameters::reviewId, std::to_string(reviewId)},
            {review_app_parameters::userId, std::to_string(userId)}
        });

    if (reviewVotes.empty())
    {
        co_return review_vote();
    }
    else
    {
        co_return reviewVotes[0];
    }
}

auto review_client::get_all_users(database_id userId, database_id reviewId) -> concurrencpp::result<std::vector<user_data>>
{
    std::vector<user_data> users = co_await get_api<std::vector<user_data>>(m_client, review_app_endpoints::users,httplib::Params
        {
            { review_app_parameters::userId, std::to_string(userId) },
            { review_app_parameters::reviewId, std::to_string(reviewId) }
        });

    co_return users;
}

auto review_client::get_all_review_activity(database_id reviewId) -> concurrencpp::result<std::vector<activity_data>>
{
    co_return co_await get_api<std::vector<activity_data>>(m_client, review_app_endpoints::activity, httplib::Params
        {
            { review_app_parameters::reviewId, std::to_string(reviewId) }
        });
}

auto review_client::get_review_file(std::filesystem::path relativeFilePath) -> concurrencpp::result<review_file_data>
{
    co_await concurrencpp::resume_on(gluten::app::get()->background_executor());

    review_file_data data;

    const httplib::Params params
    {
        { review_app_parameters::file, relativeFilePath.string() }
    };

    if (auto result = m_client->Get(review_app_endpoints::files, params, httplib::Headers()))
    {
        if (http_result_okay(result))
        {
            data.m_fileData = std::vector<unsigned char>(result.value().body.begin(), result.value().body.end());
            data.m_fileName = relativeFilePath.string();
        }
    }

    co_return data;
}

auto review_client::user_is_logged_in() -> concurrencpp::result<bool>
{
    co_return co_await get_api<bool>(m_client, review_app_endpoints::me);
}

auto review_client::user_table_is_empty() -> concurrencpp::result<bool>
{
    co_return co_await get_api<bool>(m_client, review_app_endpoints::queries, httplib::Params{ { review_app_parameters::query, review_app_queries::userTableEmptyQuery } });
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

auto review_client::post_review_version(database_id reviewId, new_transit_review_data newReviewData) -> concurrencpp::result<tl::expected<review_data, bool>>
{
    httplib::UploadFormDataItems items;

    for (const auto contextFile : newReviewData.m_contextFiles)
    {
        items.push_back(
            {
                review_app_parameters::contextFile,
                std::string(reinterpret_cast<const char*>(contextFile.m_fileData.data()), contextFile.m_fileData.size()),
                contextFile.m_fileName,
                "application/octet-stream"
            });
    }

    for (const auto reviewFile : newReviewData.m_reviewFiles)
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

    const httplib::Result postResult = m_client->Post(fmt::format("{}/{}", review_app_endpoints::reviews, reviewId), httplib::Headers(), items);

    if (http_result_okay(postResult))
    {
        review = review_app_serialization::deserialize_from_xml<review_data>(postResult.value().body);
    }

    co_return review;
}

auto review_client::post_comment(new_comment_data comment) -> concurrencpp::result<comment_data>
{
    httplib::UploadFormDataItems items = 
    {
        {
            review_app_parameters::data,
            review_app_serialization::serialize_to_xml<new_comment_data>(comment),
            "", "application/xml"
        }
    };

    co_return co_await post_form_api<comment_data>(m_client, review_app_endpoints::comments, items);
}

auto review_client::post_user(new_user_data newUser) -> concurrencpp::result<user_data>
{
    httplib::UploadFormDataItems items = 
    {
        {
            review_app_parameters::data,
            review_app_serialization::serialize_to_xml<new_user_data>(newUser),
            "", "application/xml"
        }
    };

    co_return co_await post_form_api<user_data>(m_client, review_app_endpoints::users, items);
}

auto review_client::put_review_status(database_id reviewId, review_status status) -> concurrencpp::result<void>
{
    co_return co_await put_api(m_client, review_app_endpoints::reviews, httplib::Params
        {
            { review_app_parameters::reviewId, std::to_string(reviewId) },
            { review_app_parameters::reviewStatus, std::to_string((int)status) }
        });
}

auto review_client::put_review_vote(database_id reviewId, database_id userId, review_vote vote) -> concurrencpp::result<void>
{
    co_return co_await put_api(m_client, review_app_endpoints::reviewVotes, httplib::Params
        {
            { review_app_parameters::reviewId, std::to_string(reviewId) },
            { review_app_parameters::userId, std::to_string(userId) },
            { review_app_parameters::reviewVote, std::to_string((int)vote) }
        });
}

auto review_client::put_review(review_data reviewData) -> concurrencpp::result<void>
{
    httplib::UploadFormDataItems items = 
    {
        {
            review_app_parameters::data,
            review_app_serialization::serialize_to_xml<review_data>(reviewData),
            "", "application/xml"
        }
    };

    co_return co_await put_form_api(m_client, review_app_endpoints::reviews, items);
}

auto review_client::put_review_users(database_id reviewId, std::vector<database_id> userIds) -> concurrencpp::result<void>
{
    httplib::UploadFormDataItems items = 
    {
        {
            review_app_parameters::data,
            review_app_serialization::serialize_to_xml<std::vector<database_id>>(userIds),
            "", "application/xml"
        },
        {
            review_app_parameters::reviewId,
            review_app_serialization::serialize_to_xml<database_id>(reviewId),
            "", "application/xml"
        }
    };

    co_return co_await put_form_api(m_client, review_app_endpoints::reviewUsers, items);
}

auto review_client::delete_review(database_id reviewId) -> concurrencpp::result<void>
{
    co_return co_await delete_api(m_client, review_app_endpoints::reviews, httplib::Params
        {
            { review_app_parameters::reviewId, std::to_string(reviewId) },
        });
}

auto review_client::delete_comment(database_id commentId) -> concurrencpp::result<void>
{
    co_return co_await delete_api(m_client, review_app_endpoints::comments, httplib::Params
        {
            { review_app_parameters::commentId, std::to_string(commentId) },
        });
}

auto review_client::delete_user(database_id userId) -> concurrencpp::result<void>
{
    co_return co_await delete_api(m_client, review_app_endpoints::users, httplib::Params
        {
            { review_app_parameters::userId, std::to_string(userId) },
        });
}