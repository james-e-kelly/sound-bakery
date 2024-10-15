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
        button(const char* name, bool invisible = false, const anchor_preset& anchorPreset = anchor_preset::left_top);

        bool render_element(const ImRect& parent) override;

    protected:
        auto get_element_content_size() -> ImVec2 const override;

    private:
        const char* m_name     = nullptr;
        const bool m_invisible = false;
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