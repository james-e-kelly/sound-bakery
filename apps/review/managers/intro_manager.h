#pragma once

#include "pch.h"
#include "data/user_settings_data.h"

class intro_widget;

namespace review_app_test_connect
{
    auto test_server_connection(std::string serverAddress) -> concurrencpp::result<bool>;
}

class intro_manager : public gluten::manager
{
public:
    intro_manager(gluten::app* appOwner) : gluten::manager(appOwner) {}

protected:
    auto init(gluten::app* app) -> void override;
    auto start() -> void override;
    auto tick(double deltaTime) -> void override;

private:

    concurrencpp::result<bool> m_testServerConnectionResult;
    gluten::data_source<user_settings_data> m_userSettingsData;
    std::shared_ptr<gluten::loading_popup> m_loadingPopup;
    std::shared_ptr<intro_widget> m_introWidget;
};