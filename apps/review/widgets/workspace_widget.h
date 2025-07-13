#pragma once

#include "pch.h"

class workspace_manager;

class workspace_widget : public gluten::window_widget
{
    WIDGET_CONSTRUCT_PARENT(workspace_widget, "Workspace", gluten::window_widget)

protected:
    virtual auto start_implementation() -> void override;
    virtual auto render_window_implementation() -> void override;
    virtual auto render_menu_implementation() -> void override;

private:
    auto render_content() -> void;
    auto render_list() -> void;
    auto render_left_toolbar() -> void;

    enum active_view
    {
        reviews_view,
        users_view
    };

    active_view m_activeView = reviews_view;
    std::shared_ptr<workspace_manager> m_workspaceManager;
};
