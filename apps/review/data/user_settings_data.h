#pragma once

#include "pch.h"

#include "data/user_data.h"

enum class review_app_theme
{
    dark,
    light
};

enum class review_app_view
{
    reviews,    //< Looking at projects and reviews
    users,      //< Looking at users
};

/**
 * @brief Cached user settings like the open workspace
 *
 * Stores where the workspace lives so it can be loaded upon boot automatically.
 */
struct user_settings_data
{
    std::filesystem::path m_workspaceFilePath;  //< File opened by the server
    std::string m_serverIpAddress;              //< Server address connected to by the client

    logged_in_user_data m_loggedInUser;
    review_app_theme m_theme     = review_app_theme::dark;
    review_app_view m_activeView = review_app_view::reviews;

    auto workspace_exists() const -> bool
    {
        return std::filesystem::exists(m_workspaceFilePath);
    }

    auto server_address_valid() const -> bool
    {
        return !m_serverIpAddress.empty();
    }

    template <class archive_class>
    auto serialize(archive_class& archive, const unsigned int version) -> void
    {
        archive& boost::serialization::make_nvp("Workspace", m_workspaceFilePath);

        if (version >= review_app_user_tokens)
        {
            archive& boost::serialization::make_nvp("UserLogin", m_loggedInUser);
        }

        if (version >= review_app_first_settings)
        {
            archive& boost::serialization::make_nvp("theme", m_theme);
        }

        if (version >= review_app_ip_address)
        {
            archive& BOOST_SERIALIZATION_NVP(m_serverIpAddress);
        }
    }
};

BOOST_CLASS_VERSION(user_settings_data, review_app_version_current)