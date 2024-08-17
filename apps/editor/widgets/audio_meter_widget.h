#pragma once

#include "gluten/widgets/widget.h"

class audio_meter_widget : public gluten::widget
{
public:
    WIDGET_CONSTRUCT(audio_meter_widget)

    void render() override;
};
