#pragma once

#include "gluten/widgets/widget.h"

class DetailsWidget : public gluten::widget
{
public:
    DetailsWidget(gluten::widget_subsystem* parentSubsystem) : widget(parentSubsystem) {}

    DetailsWidget(widget* parent) : widget(parent) {}

public:
    virtual void render() override;
};
