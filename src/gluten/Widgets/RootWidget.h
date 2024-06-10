#pragma once

#include "Widget.h"

class RootWidget : public Widget
{
public:
    RootWidget(WidgetSubsystem* parentSubsystem) : Widget(parentSubsystem) {}

public:
    virtual void Render() override;
};
