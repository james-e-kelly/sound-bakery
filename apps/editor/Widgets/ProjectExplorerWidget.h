#pragma once

#include "Widget.h"

class ProjectNodesWidget;

class ProjectExplorerWidget : public Widget
{
public:
    ProjectExplorerWidget(WidgetSubsystem* parentSubsystem)
        : Widget(parentSubsystem)
    {
    }

    ProjectExplorerWidget(Widget* parentWidget) : Widget(parentWidget) {}

public:
    virtual void Start() override;
    virtual void Render() override;

private:
    Widget* m_fileBrowserWidget;
    ProjectNodesWidget* m_projectNodesWidget;
};
