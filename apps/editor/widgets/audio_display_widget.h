#pragma once

#include "gluten/widgets/widget.h"
#include "sound_bakery/sound_bakery.h"

namespace sbk::engine
{
    class Sound;
}

class AudioDisplayWidget : public gluten::widget
{
public:
    AudioDisplayWidget(gluten::widget_subsystem* parentSubsystem)
        : widget(parentSubsystem)
    {
    }

    AudioDisplayWidget(widget* parent) : widget(parent) {}

public:
    virtual void render() override;

private:
    bool HasCache();
    void GenerateCache();

private:
    std::vector<std::vector<float>> m_cachedSamples;
    int m_playPixel           = 0;
    float m_drawWidth         = 0;
    float m_previousDrawWidth = 0;
    sbk::engine::Sound* m_previousSound;
};
