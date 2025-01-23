#pragma once

#include "gluten/elements/image_button.h"
#include "gluten/widgets/widget.h"

namespace gluten
{
    /**
     * @brief The base widget with titlebar and open/close icons.
     */
    class root_widget : public widget
    {
    public:
        root_widget(widget_subsystem* parentSubsystem) : widget(parentSubsystem, "Root Widget") { m_autoRenderChildren = false; }

    public:
        void start_implementation() override;
        void render_implementation() override;
        auto render_menu_implementation() -> void override;

        bool is_hovering_titlebar() { return m_hoveringTitlebar; }

    private:
        void submit_dockspace();
        void set_root_window_to_viewport();
        void draw_titlebar();

        bool m_hoveringTitlebar = false;

        std::unique_ptr<gluten::image> m_windowIcon;
        std::unique_ptr<gluten::image_button> m_windowCloseIcon;
        std::unique_ptr<gluten::image_button> m_windowMinimiseIcon;
        std::unique_ptr<gluten::image_button> m_windowMaximiseIcon;
        std::unique_ptr<gluten::image_button> m_windowRestoreIcon;
    };
}  // namespace gluten