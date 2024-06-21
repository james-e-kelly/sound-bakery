#pragma once

#include "gluten/widgets/widget.h"

class root_widget : public gluten::widget
{
public:
    root_widget(gluten::widget_subsystem* parentSubsystem) : widget(parentSubsystem) {}

public:
    virtual void render() override;
};
