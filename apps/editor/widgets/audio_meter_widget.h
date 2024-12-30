#pragma once

#include "gluten/widgets/widget.h"
#include "sound_bakery/system.h"

class sc_dsp;

class audio_meter_widget : public gluten::widget
{
public:
    WIDGET_CONSTRUCT(audio_meter_widget)

    void start_implementation() override;
    void render_implementation() override;

private:
    sc_dsp* m_meterDsp = nullptr;
    std::array<float, SC_DSP_METER_MAX_CHANNELS> m_rmsVolumes;
};
