#pragma once

#include "pch.h"

#include "app/review_database.h"
#include "data/user_data.h"
#include "data/user_settings_data.h"

enum class user_flow_type
{
    login_user,         //< Simple login
    new_user,           //< Create a new user for someone else
    new_user_and_login  //< Create a new user and login as that user
};

/**
 * @brief Handles logins or new users.
 */
class user_flow_popup : public gluten::popup_widget
{
    WIDGET_CONSTRUCT_PARENT(user_flow_popup, "User Flow", gluten::popup_widget)

public:
    auto set_flow_type(user_flow_type type) -> void;

protected:
    auto render_popup() -> void override;
    auto start_implementation() -> void override;
    auto end_implementation() -> void override;

private:
    static inline constexpr std::size_t textBufferSize = 512;

    char m_emailBuffer[textBufferSize]       = {0};
    char m_passwordBuffer[g_rawPasswordSize] = {0};
    char m_titleBuffer[textBufferSize]       = {0};
    char m_displayNameBuffer[textBufferSize] = {0};
    user_privileges m_privileges             = user_privileges::guest;

    user_flow_type m_type    = user_flow_type::login_user;
    bool m_firstUserCreation = false;

    gluten::data_source<user_settings_data> m_userSettings;

    std::string m_errorText;
    bool m_emailIsValid = false;
    concurrencpp::result<tl::expected<bool, database_error>> m_loginResult;
};