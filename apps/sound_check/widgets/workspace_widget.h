#pragma once

#include "pch.h"
#include "gluten/widgets/window_widget.h"

class workspace_widget : public gluten::window_widget
{
    WIDGET_CONSTRUCT_PARENT(workspace_widget, "Workspace", gluten::window_widget)

protected:
    virtual auto render_window_implementation() -> void override;
    auto virtual render_menu_implementation() -> void override;
};
