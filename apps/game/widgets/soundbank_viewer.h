#pragma once

#include "gluten/widgets/root_widget.h"

#include "sound_bakery/system.h"
#include "sound_chef/sound_chef_bank.h"

class soundbank_viewer_widget : public gluten::widget
{
    WIDGET_CONSTRUCT(soundbank_viewer_widget)

public:
    void start_implementation() override;
    void render_implementation() override;
    void end_implementation() override;

    void set_soundbank_to_view(const std::filesystem::path& soundbankFilePath);

private:
    sc_bank m_bank;
};
