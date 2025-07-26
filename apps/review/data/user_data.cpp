#include "user_data.h"

#include "sodium.h"

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