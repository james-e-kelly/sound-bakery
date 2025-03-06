#pragma once

#include "gluten/widgets/widget.h"

class project_nodes_widget;

class project_explorer_widget : public gluten::widget
{
public:
    WIDGET_CONSTRUCT(project_explorer_widget, "Project Explorer Widget")

    virtual void start_implementation() override;
    virtual void render_implementation() override;

private:
    std::shared_ptr<widget> m_fileBrowserWidget;
    std::shared_ptr<project_nodes_widget> m_projectNodesWidget;
};
