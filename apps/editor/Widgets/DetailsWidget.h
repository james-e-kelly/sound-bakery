#pragma once

#include "Widget.h"

class DetailsWidget : public widget
{
public:
    DetailsWidget(widget_subsystem* parentSubsystem) : widget(parentSubsystem) {}

    DetailsWidget(widget* parent) : widget(parent) {}

public:
    virtual void Render() override;
};
