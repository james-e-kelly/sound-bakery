#pragma once

#include "gluten/widgets/widget.h"

namespace gluten
{
    class root_widget : public widget
    {
    public:
        root_widget(widget_subsystem* parentSubsystem) : widget(parentSubsystem) {}

    public:
        virtual void render() override;
    };
}  // namespace gluten