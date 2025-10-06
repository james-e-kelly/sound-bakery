#include "review_server.h"

#include "app/review_app.h"
#include "app/review_database.h"
#include "managers/workspace_manager.h"

#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/applink.c>    // required for file writing

namespace
{
    auto generate_key() -> EVP_PKEY*
    {
        if (EVP_PKEY* const pkey = EVP_PKEY_new())
        {
            RSA* const rsa = RSA_generate_key(2048, RSA_F4, NULL, NULL);
            
            if (EVP_PKEY_assign_RSA(pkey, rsa))
            {
                return pkey;
            }
            EVP_PKEY_free(pkey);
        }
        return nullptr;
    }

    auto generate_x509(EVP_PKEY* pkey) -> X509*
    {
        X509* const x509 = X509_new();
        if (!x509)
        {
            return nullptr;
        }

        ASN1_INTEGER_set(X509_get_serialNumber(x509), 1);

        X509_gmtime_adj(X509_get_notBefore(x509), 0);
        X509_gmtime_adj(X509_get_notAfter(x509), std::chrono::duration_cast<std::chrono::seconds>(std::chrono::years(1)).count());

        X509_set_pubkey(x509, pkey);

        X509_NAME* const name = X509_get_subject_name(x509);

        X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, (unsigned char*)"GB", -1, -1, 0);
        X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC, (unsigned char*)"SoundBakery", -1, -1, 0);
        X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (unsigned char*)"localhost", -1, -1, 0);

        X509_set_issuer_name(x509, name);

        if (!X509_sign(x509, pkey, EVP_sha1()))
        {
            X509_free(x509);
            return nullptr;
        }

        return x509;
    }

    auto write_to_disk(EVP_PKEY* pkey, X509* x509, const std::filesystem::path& keyPath, const std::filesystem::path& certPath) -> bool
    {
        if (FILE* pkey_file = fopen(keyPath.string().c_str(), "wb"))
        {
            bool ret = PEM_write_PrivateKey(pkey_file, pkey, NULL, NULL, 0, NULL, NULL);
            fclose(pkey_file);

            if (ret)
            {
                if (FILE* x509_file = fopen(certPath.string().c_str(), "wb"))
                {
                    ret = PEM_write_X509(x509_file, x509);
                    fclose(x509_file);
                    return true;
                }
            }
        }

        return false;
    }

    template<typename data_type>
    auto add_database_get_endpoint(const std::unique_ptr<httplib::SSLServer>& server, const std::string& pattern, std::function<review_database::database_result<data_type>(std::shared_ptr<review_database> database, std::string userToken, const httplib::Request& request)> databaseFunction)
    {
        server->Get(pattern, [function = std::move(databaseFunction)](const httplib::Request& request, httplib::Response& response) 
            {
                if (auto database = review_app::get_review_database())
                {
                    const auto databaseResult = function(database, httplib::get_bearer_token_auth(request), request).get();

                    if (databaseResult.has_value())
                    {
                        const data_type& data = databaseResult.value();
                        const std::string serializedXml = review_app_serialization::serialize_to_xml(data);
                        response.set_content(serializedXml, "application/xml");
                    }
                    else
                    {
                        const auto error = databaseResult.error();

                        if (error.m_errorCode == database_error_code::unauthorized)
                        {
                            response.status = httplib::StatusCode::Unauthorized_401;
                        }
                        else
                        {
                            response.status = httplib::StatusCode::InternalServerError_500;
                        }
                    }
                }
                else
                {
                    response.status = httplib::StatusCode::InternalServerError_500;
                }
            });
    }

    auto add_database_post_endpoint(const std::unique_ptr<httplib::SSLServer>& server, const std::string& pattern, std::function<void(std::shared_ptr<review_database> database, std::string userToken, const httplib::Request& request)> databaseFunction)
    {
        server->Post(pattern, [function = std::move(databaseFunction)](const httplib::Request& request, httplib::Response& response) 
            {
                if (auto database = review_app::get_review_database())
                {
                    function(database, httplib::get_bearer_token_auth(request), request);

                    response.status = httplib::StatusCode::NoContent_204;
                }
                else
                {
                    response.status = httplib::StatusCode::InternalServerError_500;
                }
            });
    }

    auto add_database_post_form_endpoint(const std::unique_ptr<httplib::SSLServer>& server, const std::string& pattern, std::function<void(std::shared_ptr<review_database> database, std::string userToken, const httplib::Request& request, httplib::Response& response)> databaseFunction)
    {
        server->Post(pattern, [function = std::move(databaseFunction)](const httplib::Request& request, httplib::Response& response) 
            {
                if (auto database = review_app::get_review_database())
                {
                    function(database, httplib::get_bearer_token_auth(request), request, response);

                    if (response.body.empty())
                    {
                        response.status = httplib::StatusCode::NoContent_204;
                    }
                }
                else
                {
                    response.status = httplib::StatusCode::InternalServerError_500;
                }
            });
    }

    auto add_database_put_endpoint(const std::unique_ptr<httplib::SSLServer>& server, const std::string& pattern, std::function<void(std::shared_ptr<review_database> database, std::string userToken, const httplib::Request& request, httplib::Response& response)> databaseFunction)
    {
        server->Put(pattern, [function = std::move(databaseFunction)](const httplib::Request& request, httplib::Response& response) 
            {
                if (auto database = review_app::get_review_database())
                {
                    function(database, httplib::get_bearer_token_auth(request), request, response);

                    if (response.status <= 0)
                    {
                        response.status = httplib::StatusCode::OK_200;
                    }
                }
                else
                {
                    response.status = httplib::StatusCode::InternalServerError_500;
                }
            });
    }

    auto add_database_delete_endpoint(const std::unique_ptr<httplib::SSLServer>& server, const std::string& pattern, std::function<void(std::shared_ptr<review_database> database, std::string userToken, const httplib::Request& request, httplib::Response& response)> databaseFunction)
    {
        server->Delete(pattern, [function = std::move(databaseFunction)](const httplib::Request& request, httplib::Response& response) 
            {
                if (auto database = review_app::get_review_database())
                {
                    function(database, httplib::get_bearer_token_auth(request), request, response);

                    if (response.status <= 0)
                    {
                        response.status = httplib::StatusCode::OK_200;
                    }
                }
                else
                {
                    response.status = httplib::StatusCode::InternalServerError_500;
                }
            });
    }
}

review_server::review_server(gluten::app* app, const std::filesystem::path& workspacePath)
    : gluten::manager(app)
{
    std::error_code errorCode;
    BOOST_ASSERT_MSG(std::filesystem::is_directory(workspacePath, errorCode), "workspacePath must be a directory and not a file!");

    const std::filesystem::path certPath = workspacePath / "cert.pem";
    const std::filesystem::path keyPath  = workspacePath / "key.pem";

    if (!std::filesystem::exists(certPath, errorCode) || !std::filesystem::exists(keyPath, errorCode))
    {
        std::filesystem::remove(certPath, errorCode);
        std::filesystem::remove(keyPath, errorCode);

        EVP_PKEY* const key     = generate_key();
        X509* const cert        = generate_x509(key);
        write_to_disk(key, cert, keyPath, certPath);
    }

    BOOST_ASSERT_MSG(std::filesystem::exists(certPath) && std::filesystem::exists(keyPath), "Cert or key file does not exist!");

    m_server = std::make_unique<httplib::SSLServer>(certPath.string().c_str(), keyPath.string().c_str());

    // GET

    add_database_get_endpoint<bool>(m_server, review_app_endpoints::me, [](std::shared_ptr<review_database> database, std::string userToken, const httplib::Request& request)
        {
            if (request.has_param(review_app_parameters::activityType))
            {
                const activity_type activityType = (activity_type)std::stoi(request.get_param_value(review_app_parameters::activityType));

                return database->user_can_perform_action(userToken, activityType);
            }
            else
            {
                return database->user_has_privilege(userToken, user_privileges::guest);
            }
        });

    add_database_get_endpoint<workspace_data>(m_server, review_app_endpoints::workspace, [](std::shared_ptr<review_database> database, std::string userToken, const httplib::Request& request)
        {
            return database->get_workspace(userToken);
        });

    add_database_get_endpoint<std::vector<project_data>>(m_server, review_app_endpoints::projects, [](std::shared_ptr<review_database> database, std::string userToken, const httplib::Request& request)
        {
            return database->get_all_projects(userToken);
        });

    add_database_get_endpoint<std::vector<review_data>>(m_server, review_app_endpoints::reviews, [](std::shared_ptr<review_database> database, std::string userToken, const httplib::Request& request)
        {
            database_id projectId = 0;
            
            if (request.has_param(review_app_parameters::projectId))
            {
                projectId = std::stol(request.get_param_value(review_app_parameters::projectId));
            }

            return database->get_all_reviews(projectId, userToken);
        });

    add_database_get_endpoint<std::vector<review_vote>>(m_server, review_app_endpoints::reviewVotes, [](std::shared_ptr<review_database> database, std::string userToken, const httplib::Request& request)
        {
            database_id reviewId = 0;
            database_id userId = 0;
            
            if (request.has_param(review_app_parameters::reviewId))
            {
                reviewId = std::stol(request.get_param_value(review_app_parameters::reviewId));
            }

            if (request.has_param(review_app_parameters::userId))
            {
                userId = std::stol(request.get_param_value(review_app_parameters::userId));
            }

            return database->get_review_votes(reviewId, userId, userToken);
        });

    add_database_get_endpoint<std::vector<user_data>>(m_server, review_app_endpoints::users, [](std::shared_ptr<review_database> database, std::string userToken, const httplib::Request& request)
        {
            database_id userId = 0;

            if (request.has_param(review_app_parameters::userId))
            {
                userId = std::stol(request.get_param_value(review_app_parameters::userId));
            }

            if (request.has_param(review_app_parameters::reviewId))
            {
                return database->get_review_users(std::stol(request.get_param_value(review_app_parameters::reviewId)), userToken);
            }
            else
            {
                return database->get_all_users(userId, userToken);
            }
        });

    add_database_get_endpoint<std::vector<comment_data>>(m_server, review_app_endpoints::comments, [](std::shared_ptr<review_database> database, std::string userToken, const httplib::Request& request)
        {
            database_id reviewId = 0;

            if (request.has_param(review_app_parameters::reviewId))
            {
                reviewId = std::stol(request.get_param_value(review_app_parameters::reviewId));
            }

            return database->get_all_comments(reviewId, 0, userToken);
        });

    add_database_get_endpoint<std::vector<activity_data>>(m_server, review_app_endpoints::activity, [](std::shared_ptr<review_database> database, std::string userToken, const httplib::Request& request)
        {
            database_id reviewId = 0;

            if (request.has_param(review_app_parameters::reviewId))
            {
                reviewId = std::stol(request.get_param_value(review_app_parameters::reviewId));
            }

            return database->get_all_review_activity(reviewId, userToken);
        });

    // POST

    add_database_post_endpoint(m_server, review_app_endpoints::projects, [](std::shared_ptr<review_database> database, std::string userToken, const httplib::Request& request)
        {
            const std::string name = request.get_param_value(review_app_parameters::name);
            const std::string description = request.get_param_value(review_app_parameters::description);

            return database->create_project(name, description, userToken);
        });

    add_database_post_form_endpoint(m_server, review_app_endpoints::reviews, [](std::shared_ptr<review_database> database, std::string userToken, const httplib::Request& request, httplib::Response& response)
        {
            new_transit_review_data newReviewTransitData = review_app_serialization::deserialize_from_xml<new_transit_review_data>(request.form.get_field(review_app_parameters::data));
            database_id projectId = std::stoll(request.form.get_field(review_app_parameters::projectId));
            auto contextFiles = request.form.get_files(review_app_parameters::contextFile);
            auto reviewFiles = request.form.get_files(review_app_parameters::reviewFile);

            for (const httplib::FormData& doc : contextFiles) 
            {
                newReviewTransitData.m_contextFiles.push_back(
                    {
                        doc.filename,
                        std::vector<uint8_t>(doc.content.begin(), doc.content.end())
                    });
            }

            for (const httplib::FormData& doc : reviewFiles) 
            {
                newReviewTransitData.m_reviewFiles.push_back(
                    {
                        doc.filename,
                        std::vector<uint8_t>(doc.content.begin(), doc.content.end())
                    });
            }

            const auto newReviewResult = database->create_review(projectId, newReviewTransitData, userToken).get();

            review_data newReviewData;

            if (newReviewResult.has_value())
            {
                newReviewData = newReviewResult.value();
            }

            response.set_content(review_app_serialization::serialize_to_xml<review_data>(newReviewData), "application/xml");
        });

    add_database_post_form_endpoint(m_server, review_app_endpoints::comments, [](std::shared_ptr<review_database> database, std::string userToken, const httplib::Request& request, httplib::Response& response)
        {
            new_comment_data newCommentData = review_app_serialization::deserialize_from_xml<new_comment_data>(request.form.get_field(review_app_parameters::data));

            const auto newCommentResult = database->create_comment(newCommentData, userToken).get();

            comment_data createdComment;

            if (newCommentResult.has_value())
            {
                createdComment = newCommentResult.value();
            }

            response.set_content(review_app_serialization::serialize_to_xml<comment_data>(createdComment), "application/xml");
        });

    add_database_post_form_endpoint(m_server, review_app_endpoints::users, [](std::shared_ptr<review_database> database, std::string userToken, const httplib::Request& request, httplib::Response& response)
        {
            new_user_data newUserData = review_app_serialization::deserialize_from_xml<new_user_data>(request.form.get_field(review_app_parameters::data));

            const auto newUserResult = database->create_user(newUserData, userToken).get();

            user_data createdUser;

            if (newUserResult.has_value())
            {
                createdUser = newUserResult.value();
            }

            response.set_content(review_app_serialization::serialize_to_xml<user_data>(createdUser), "application/xml");
        });

    add_database_post_form_endpoint(m_server, review_app_endpoints::login, [](std::shared_ptr<review_database> database, std::string userToken, const httplib::Request& request, httplib::Response& response)
        {
            login_request_data loginRequestData = review_app_serialization::deserialize_from_xml<login_request_data>(request.form.get_field(review_app_parameters::data));
            
            const auto loggedInUserResult = database->login_user(loginRequestData).get();

            logged_in_user_data loggedInUserData;

            if (loggedInUserResult.has_value())
            {
                loggedInUserData = loggedInUserResult.value();
                
                response.set_content(review_app_serialization::serialize_to_xml<logged_in_user_data>(loggedInUserData), "application/xml");
            }
        });

    // PUT

    add_database_put_endpoint(m_server, review_app_endpoints::reviews, [](std::shared_ptr<review_database> database, std::string userToken, const httplib::Request& request, httplib::Response& response)
        {
            if (request.has_param(review_app_parameters::reviewId))
            {
                const database_id reviewId = std::stol(request.get_param_value(review_app_parameters::reviewId));
                
                if (request.has_param(review_app_parameters::reviewStatus))
                {
                    const review_status status = (review_status)std::stoi(request.get_param_value(review_app_parameters::reviewStatus));
                    
                    database->set_review_status(reviewId, status, userToken).get();
                }
            }
            else if (request.form.has_field(review_app_parameters::data))
            {
                review_data reviewData = review_app_serialization::deserialize_from_xml<review_data>(request.form.get_field(review_app_parameters::data));

                database->update_review(reviewData, userToken);
            }
        });

    add_database_put_endpoint(m_server, review_app_endpoints::reviewVotes, [](std::shared_ptr<review_database> database, std::string userToken, const httplib::Request& request, httplib::Response& response)
        {
            database_id reviewId = 0;
            database_id userId = 0;
            review_vote vote = review_vote::no_vote;

            if (request.has_param(review_app_parameters::reviewId))
            {
                reviewId = std::stol(request.get_param_value(review_app_parameters::reviewId));
            }

            if (request.has_param(review_app_parameters::userId))
            {
                userId = std::stol(request.get_param_value(review_app_parameters::userId));
            }

            if (request.has_param(review_app_parameters::reviewVote))
            {
                vote = (review_vote)std::stoi(request.get_param_value(review_app_parameters::reviewVote));
            }

            database->set_review_vote(reviewId, userId, vote, userToken).get();
        });

    add_database_put_endpoint(m_server, review_app_endpoints::reviewUsers, [](std::shared_ptr<review_database> database, std::string userToken, const httplib::Request& request, httplib::Response& response)
        {
            if (request.form.has_field(review_app_parameters::data) && request.form.has_field(review_app_parameters::reviewId))
            {
                const database_id reviewId = review_app_serialization::deserialize_from_xml<database_id>(request.form.get_field(review_app_parameters::reviewId));
                const std::vector<database_id> reviewUsers = review_app_serialization::deserialize_from_xml<std::vector<database_id>>(request.form.get_field(review_app_parameters::data));

                database->set_review_users(reviewId, reviewUsers, userToken);
            }
        });

    // DELETE

    add_database_delete_endpoint(m_server, review_app_endpoints::reviews, [](std::shared_ptr<review_database> database, std::string userToken, const httplib::Request& request, httplib::Response& response)
        {
            database_id reviewId = 0;
            
            if (request.has_param(review_app_parameters::reviewId))
            {
                reviewId = std::stol(request.get_param_value(review_app_parameters::reviewId));
            }

            if (reviewId > 0)
            {
                database->delete_review(reviewId, userToken).get();
            }
            else
            {
                response.status = httplib::StatusCode::BadRequest_400;
            }
        });

    add_database_delete_endpoint(m_server, review_app_endpoints::comments, [](std::shared_ptr<review_database> database, std::string userToken, const httplib::Request& request, httplib::Response& response)
        {
            database_id commentId = 0;
            
            if (request.has_param(review_app_parameters::commentId))
            {
                commentId = std::stol(request.get_param_value(review_app_parameters::commentId));
            }

            if (commentId > 0)
            {
                database->delete_comment(commentId, userToken).get();
            }
            else
            {
                response.status = httplib::StatusCode::BadRequest_400;
            }
        });
}

auto review_server::start() -> void
{
    get_app()->background_executor()->submit([this]() 
        {
            m_server->listen("localhost", 8080);
        });
}

auto review_server::exit() -> void
{
    if (m_server)
    {
        m_server->stop();
    }
}