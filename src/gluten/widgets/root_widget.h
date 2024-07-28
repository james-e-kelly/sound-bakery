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
        virtual void render_menu() {}

        bool is_hovering_titlebar() { return m_hoveringTitlebar; }

    private:
        void submit_dockspace();
        void set_root_window_to_viewport();
        void draw_titlebar();

        bool m_hoveringTitlebar = false;
    };
}  // namespace gluten