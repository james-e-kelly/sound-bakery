#pragma once

#include "Widget.h"

class PlayerWidget : public widget
{
public:
    PlayerWidget(widget_subsystem* parentSubsystem) : widget(parentSubsystem) {}

    PlayerWidget(widget* parentWidget) : widget(parentWidget) {}

public:
    virtual void Render() override;
    virtual void start() override;
};
