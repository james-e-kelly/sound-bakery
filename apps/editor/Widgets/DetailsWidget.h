#pragma once

#include "Widget.h"

class DetailsWidget : public Widget
{
public:
    DetailsWidget(WidgetSubsystem* parentSubsystem) : Widget(parentSubsystem) {}

    DetailsWidget(Widget* parent) : Widget(parent) {}

public:
    virtual void Render() override;
};
