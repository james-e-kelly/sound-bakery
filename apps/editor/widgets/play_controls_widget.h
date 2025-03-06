#pragma once

#include "gluten/widgets/widget.h"

class player_widget : public gluten::widget
{
public:
    WIDGET_CONSTRUCT(player_widget, "Player###Player Widget")

    virtual void render_implementation() override;
    virtual void start_implementation() override;

private:
    void play_selected();
    void stop_selected();
    void toggle_play_selected();
};
