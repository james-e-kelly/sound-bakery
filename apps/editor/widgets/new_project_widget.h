#pragma once

#include "gluten/widgets/widget.h"

class new_project_widget : public gluten::widget
{
public:
    WIDGET_CONSTRUCT(new_project_widget, "New Project Wizard")

    auto open_new_project_popup() -> void;
    auto close_new_project_popup() -> void;

protected:
    virtual void start_implementation() override;
    virtual void render_implementation() override;
};
