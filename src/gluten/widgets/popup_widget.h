#pragma once

#include "gluten/widgets/widget.h"

namespace gluten
{
    class popup_widget : public widget
    {
    public:
        popup_widget(widget* parent, const std::string& name) : widget(parent, name) {}
        popup_widget(widget_subsystem* subsystem, const std::string& name) : widget(subsystem, name) {}

        auto open_popup() -> void;
        auto close_popup() -> void;

    protected:
        virtual auto start_implementation() -> void override;
        virtual auto render_popup() -> void = 0;

    private:
        virtual auto render_implementation() -> void override;
    };
}  // namespace gluten