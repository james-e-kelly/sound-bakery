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

    class confirmation_popup : public popup_widget
    {
    public:
        confirmation_popup(widget* parent, const std::string& title, const std::function<void()>& onConfirm) : popup_widget(parent, title), m_onConfirm(onConfirm) {}
        confirmation_popup(widget_subsystem* subsystem, const std::function<void()>& onConfirm)
            : popup_widget(subsystem, "Confirm"), m_onConfirm(onConfirm)
        {
        }

    protected:
        auto render_popup() -> void override;

    private:
        std::function<void()> m_onConfirm;
    };
}  // namespace gluten