#pragma once

#include "gluten/elements/element.h"

namespace gluten
{
    /**
     * @brief Renders a button (optionally invisible) and returns true if it is pressed.
     */
    class button : public element
    {
    public:
        button() = delete;
        button(const char* name, bool invisible, const anchor_preset& anchorPreset, const button_style& buttonStyle = button_style::secondary);

        auto set_button_style(button_style style) -> button&;

        bool render_element(const element_render_info& renderInfo) override;
        auto get_element_content_size(const ImVec2& parentSize = ImVec2(0,0)) -> ImVec2 const override;

    private:
        const char* m_name     = nullptr;
        const bool m_invisible = false;
        std::optional<button_style> m_buttonStyle;
    };

    /**
     * @brief Specialised button that is always visible and takes the full size of the box given by the toolbar.
     */
    class toolbar_button : public button
    {
    public:
        toolbar_button(const char* name) : button(name, false, anchor_preset::stretch_full) {}
    };
}  // namespace gluten