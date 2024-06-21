#pragma once

#include "gluten/widgets/widget.h"

class ProjectNodesWidget;

class ProjectExplorerWidget : public gluten::widget
{
public:
    ProjectExplorerWidget(gluten::widget_subsystem* parentSubsystem)
        : widget(parentSubsystem)
    {
    }

    ProjectExplorerWidget(widget* parentWidget) : widget(parentWidget) {}

public:
    virtual void start() override;
    virtual void render() override;

private:
    widget* m_fileBrowserWidget;
    ProjectNodesWidget* m_projectNodesWidget;
};
