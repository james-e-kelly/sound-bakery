#pragma once

#include "pch.h"

class soundbank_viewer_widget;

class game_app final : public gluten::app
{
public:
    void post_init() override;

    void open_soundbank();

private:
    soundbank_viewer_widget* m_soundbankViewerWidget = nullptr;
};