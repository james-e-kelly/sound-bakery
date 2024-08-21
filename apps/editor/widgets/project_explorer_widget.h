#pragma once

#include "gluten/widgets/widget.h"

class project_nodes_widget;

class project_explorer_widget : public gluten::widget
{
public:
    project_explorer_widget(gluten::widget_subsystem* parentSubsystem) : widget(parentSubsystem) {}

    project_explorer_widget(widget* parentWidget) : widget(parentWidget) {}

public:
    virtual void start() override;
    virtual void render() override;

private:
    widget* m_fileBrowserWidget;
    project_nodes_widget* m_projectNodesWidget;
};
