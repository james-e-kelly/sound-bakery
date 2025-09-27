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

    template <typename T>
    std::string serialize_to_xml(const T& data)
    {
        std::ostringstream outputStream;
        {
            boost::archive::xml_oarchive archive(outputStream);
            archive & BOOST_SERIALIZATION_NVP(data);
        }
        return outputStream.str();
    }

    template<typename data_type>
    auto add_database_endpoint(const std::unique_ptr<httplib::SSLServer>& server, const std::string& pattern, std::function<review_database::database_result<data_type>(std::shared_ptr<review_database> database, std::string userToken, const httplib::Request& request)> databaseFunction)
    {
        server->Get(pattern, [function = std::move(databaseFunction)](const httplib::Request& request, httplib::Response& response) 
            {
                if (auto database = review_app::get_review_database())
                {
                    const auto databaseResult = function(database, httplib::get_bearer_token_auth(request), request).get();

                    if (databaseResult.has_value())
                    {
                        response.set_content(serialize_to_xml(databaseResult.value()), "application/xml");
                    }
                    else
                    {
                        response.status = httplib::StatusCode::InternalServerError_500;
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

    add_database_endpoint<std::vector<project_data>>(m_server, review_app_api::getAllProjects, [](std::shared_ptr<review_database> database, std::string userToken, const httplib::Request& request)
        {
            return database->get_all_projects(userToken);
        });

    add_database_endpoint<std::string>(m_server, review_app_api::getworkspaceName, [](std::shared_ptr<review_database> database, std::string userToken, const httplib::Request& request)
        {
            return database->get_workspace_name(userToken);
        });

    add_database_endpoint<std::vector<review_data>>(m_server, review_app_api::getAllReviews, [](std::shared_ptr<review_database> database, std::string userToken, const httplib::Request& request)
        {
            database_id projectId = 0;
            
            if (request.has_param(review_app_parameters::projectId))
            {
                projectId = std::stol(request.get_param_value(review_app_parameters::projectId));
            }

            return database->get_all_reviews(projectId, userToken);
        });

    add_database_endpoint<review_vote>(m_server, review_app_api::getReviewVote, [](std::shared_ptr<review_database> database, std::string userToken, const httplib::Request& request)
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

            return database->get_review_vote(reviewId, userId, userToken);
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