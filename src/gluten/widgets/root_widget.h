#pragma once

#include "widget.h"

class root_widget : public widget
{
public:
    root_widget(widget_subsystem* parentSubsystem) : widget(parentSubsystem) {}

public:
    virtual void render() override;
};