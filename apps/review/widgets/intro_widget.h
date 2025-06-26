#pragma once

#include "pch.h"

#include "gluten/widgets/window_widget.h"
#include "gluten/widgets/popup_widget.h"

class create_workspace_popup : public gluten::popup_widget
{
public:
    create_workspace_popup(gluten::widget* parent) : gluten::popup_widget(parent, "Create Workspace"){}

private:
    virtual auto render_popup() -> void override;
};

class intro_widget : public gluten::window_widget
{
    WIDGET_CONSTRUCT_PARENT(intro_widget, "Setup", gluten::window_widget)

protected:
    virtual auto start_implementation() -> void override;
    virtual auto render_window_implementation() -> void override;

private:
    std::shared_ptr<create_workspace_popup> m_createWorkspacePopup;
};
