#pragma once

#include "gluten/widgets/widget.h"

class player_widget : public gluten::widget
{
public:
    player_widget(gluten::widget_subsystem* parentSubsystem) : widget(parentSubsystem) {}

    player_widget(widget* parentWidget) : widget(parentWidget) {}

public:
    virtual void render() override;
    virtual void start() override;

private:
    void play_selected();
    void stop_selected();
    void toggle_play_selected();
};
