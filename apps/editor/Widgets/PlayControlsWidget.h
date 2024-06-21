#pragma once

#include "gluten/widgets/widget.h"

class PlayerWidget : public gluten::widget
{
public:
    PlayerWidget(gluten::widget_subsystem* parentSubsystem) : widget(parentSubsystem) {}

    PlayerWidget(widget* parentWidget) : widget(parentWidget) {}

public:
    //virtual void render() override;
    //virtual void start() override;
};
