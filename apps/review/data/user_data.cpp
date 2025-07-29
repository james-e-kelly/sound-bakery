#include "user_data.h"

#include "sodium.h"

auto get_user_privileges_string(user_privileges privilege) -> std::string
{
    std::string result;

    switch (privilege)
    {
        case user_privileges::guest:
            result = "Guest";
            break;
        case user_privileges::user:
            result = "User";
            break;
        case user_privileges::admin:
            result = "Admin";
            break;
        default:
            break;
    }

    return result;
}

new_user_data::new_user_data()
{
	sodium_mlock(m_rawPassword.data(), g_rawPasswordSize);
}

new_user_data::~new_user_data()
{
	sodium_munlock(m_rawPassword.data(), g_rawPasswordSize);
}

login_request_data::login_request_data()
{
	sodium_mlock(m_rawPassword.data(), g_rawPasswordSize);
}

login_request_data::~login_request_data()
{
	sodium_munlock(m_rawPassword.data(), g_rawPasswordSize);
}