#pragma once

#include "gluten/widgets/root_widget.h"

class root_widget : public gluten::root_widget
{
public:
    root_widget(gluten::widget_subsystem* parentSubsystem) : gluten::root_widget(parentSubsystem) {}

public:
    virtual void render_menu() override;
};
