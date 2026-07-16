#pragma once

#include "pch.h"

#include "app/review_database.h"

class server_widget;

class review_server : public gluten::manager
{
public:
    review_server(gluten::app* app, const std::filesystem::path& workspacePath);
    ~review_server() = default;

    auto start() -> void override;
    auto exit() -> void override;

private:
    std::unique_ptr<httplib::SSLServer> m_server;
    std::shared_ptr<server_widget> m_serverWidget;
    std::shared_ptr<review_database> m_database;
};

class server_widget : public gluten::window_widget
{
    WIDGET_CONSTRUCT_PARENT(server_widget, "Server Widget", gluten::window_widget)

protected:
    auto render_menu_implementation() -> void override;
    auto render_window_implementation() -> void override;
};