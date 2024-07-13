#pragma once

#include "gluten/widgets/widget.h"

class details_widget : public gluten::widget
{
public:
    details_widget(gluten::widget_subsystem* parentSubsystem) : widget(parentSubsystem) {}

    details_widget(widget* parent) : widget(parent) {}

public:
    virtual void render() override;
};
