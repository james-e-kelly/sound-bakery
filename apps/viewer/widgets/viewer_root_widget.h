#pragma once

#include "gluten/widgets/root_widget.h"

class viewer_root_widget : public gluten::root_widget
{
public:
    viewer_root_widget(gluten::widget_subsystem* parentSubsystem) : gluten::root_widget(parentSubsystem) {}
};