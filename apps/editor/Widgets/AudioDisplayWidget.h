#pragma once

#include "Widget.h"

#include "sound_bakery/sound_bakery.h"

namespace SB::Engine
{
    class Sound;
}

class AudioDisplayWidget : public Widget
{
public:
    AudioDisplayWidget(WidgetSubsystem* parentSubsystem)
    : Widget(parentSubsystem) {}
    
    AudioDisplayWidget(Widget* parent)
    : Widget(parent) {}
    
public:
    virtual void Render() override;
    
private:
    bool HasCache();
    void GenerateCache();

private:
    std::vector<std::vector<float>> m_cachedSamples;
    int m_playPixel = 0;
    float m_drawWidth = 0;
    float m_previousDrawWidth = 0;
    SB::Engine::Sound* m_previousSound;
};
