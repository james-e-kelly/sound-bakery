#pragma once

#include "Widget.h"

class ProjectNodesWidget;

class ProjectExplorerWidget : public widget
{
public:
    ProjectExplorerWidget(widget_subsystem* parentSubsystem)
        : widget(parentSubsystem)
    {
    }

    ProjectExplorerWidget(widget* parentWidget) : widget(parentWidget) {}

public:
    virtual void start() override;
    virtual void Render() override;

private:
    widget* m_fileBrowserWidget;
    ProjectNodesWidget* m_projectNodesWidget;
};
