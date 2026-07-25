#pragma once

#include "sound_bakery/system.h"

#include "gluten/widgets/widget.h"

class sc_dsp;

class audio_meter_widget : public gluten::widget
{
public:
    WIDGET_CONSTRUCT(audio_meter_widget, "Audio Meter Widget")

    void start_implementation() override;
    void render_implementation() override;

private:
    sc_dsp* m_meterDsp = nullptr;
    std::array<float, SC_DSP_METER_MAX_CHANNELS> m_rmsVolumes;
};
