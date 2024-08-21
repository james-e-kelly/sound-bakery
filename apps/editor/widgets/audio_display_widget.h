#pragma once

#include "gluten/widgets/widget.h"
#include "sound_bakery/sound_bakery.h"

namespace sbk::engine
{
    class sound;
}

class audio_display_widget : public gluten::widget
{
public:
    audio_display_widget(gluten::widget_subsystem* parentSubsystem) : widget(parentSubsystem) {}

    audio_display_widget(widget* parent) : widget(parent) {}

public:
    virtual void render() override;

private:
    bool has_cache();
    void generate_cache();

private:
    std::vector<std::vector<float>> m_cachedSamples;
    int m_playPixel           = 0;
    float m_drawWidth         = 0;
    float m_previousDrawWidth = 0;
    sbk::engine::sound* m_previousSound;
};
