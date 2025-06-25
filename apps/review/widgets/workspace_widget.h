#pragma once

#include "pch.h"
#include "gluten/widgets/window_widget.h"

class workspace_widget : public gluten::window_widget
{
    WIDGET_CONSTRUCT_PARENT(workspace_widget, "Workspace", gluten::window_widget)

protected:
    virtual auto start_implementation() -> void override;
    virtual auto render_window_implementation() -> void override;
    void render_content();
    void render_list();
    void render_left_toolbar();
    virtual auto render_menu_implementation() -> void override;

private:
    enum active_view
    {
        reviews_view,
        users_view
    };

    active_view m_activeView = reviews_view;
};
