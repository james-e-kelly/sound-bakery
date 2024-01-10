#pragma once

#include "Widget.h"

class PlayerWidget : public Widget
{
public:
    PlayerWidget(WidgetSubsystem* parentSubsystem)
    : Widget(parentSubsystem) {}
    
    PlayerWidget(Widget* parentWidget)
    : Widget(parentWidget) {}
    
public:
    virtual void Render() override;
    virtual void Start() override;
};
