#pragma once

#include "pch.h"

constexpr inline std::size_t g_rawPasswordSize = 256U;  //< Set to an exteremly high value but _just_ in case someone needs it

enum class user_privileges
{
    guest = 0,  //< View only
    user = 1,   //< Create reviews and delete items they created
    admin = 2   //< Can create anything and delete anything
};

auto get_user_privileges_string(user_privileges privilege) -> std::string;

/**
 * @note Raw password is memory locked and wiped during destruction
 */
struct new_user_data
{
    new_user_data();
    ~new_user_data();

    std::string m_displayName;
    std::string m_title;            //< Audio Programmer, Sound Designer, etc.
    std::string m_email;
    std::array<char, g_rawPasswordSize> m_rawPassword;
    user_privileges m_requestedPrivileges = user_privileges::guest;
};

struct logged_in_user_data
{
    std::string m_displayName;
    std::string m_title;
    std::string m_email;
    std::string m_sessionToken;                             //< Sent to the server for authentication
    user_privileges m_privileges = user_privileges::guest;  //< User side privileges for quickly changing the UI. But the server will still authenticate

    template <class archive_class>
    auto serialize(archive_class& archive, const unsigned int version) -> void
    {
        archive & boost::serialization::make_nvp("token", m_sessionToken);
        archive & boost::serialization::make_nvp("email", m_email);
        archive & boost::serialization::make_nvp("privileges", m_privileges);
    }
};

struct user_data
{
    std::string m_displayName;
    std::string m_title;
    std::string m_email;
    std::string m_createdAt;
    user_privileges m_privileges = user_privileges::guest;
};

/**
 * @note Raw password is memory locked and wiped during destruction
 */
struct login_request_data
{
    login_request_data();
    ~login_request_data();

    std::string m_email;
    std::array<char, g_rawPasswordSize> m_rawPassword;     //< The database will hash the password as the salt lives in the database and cannot be known before
};