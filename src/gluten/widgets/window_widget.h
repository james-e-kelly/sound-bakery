#pragma once

#include "gluten/widgets/widget.h"

namespace gluten
{
    class window_widget : public widget
    {
    public:
        window_widget(widget* parent, const std::string& name) : widget(parent, name) {}
        window_widget(widget_subsystem* parentSubsystem, const std::string& name) : widget(parentSubsystem, name) { }

    protected:
        virtual auto render_window_implementation() -> void {}

    private:
        virtual auto render_implementation() -> void override final;
    };
}  // namespace gluten