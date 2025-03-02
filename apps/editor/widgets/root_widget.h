#pragma once

#include "gluten/widgets/root_widget.h"

class root_widget : public gluten::root_widget
{
public:
    root_widget(gluten::widget_subsystem* parentSubsystem) : gluten::root_widget(parentSubsystem) {}

protected:
    auto render_menu_implementation() -> void override;
    auto start_implementation() -> void override;

private:
    void render_about_window(bool& showAbout);
};