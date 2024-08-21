#pragma once

#include "gluten/widgets/widget.h"

class sc_dsp;

class audio_meter_widget : public gluten::widget
{
public:
    WIDGET_CONSTRUCT(audio_meter_widget)

    void start() override;
    void render() override;

private:
    sc_dsp* m_meterDsp = nullptr;
};
