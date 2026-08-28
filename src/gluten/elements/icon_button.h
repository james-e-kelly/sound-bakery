#pragma once

#include "gluten/elements/button.h"
#include "gluten/elements/text.h"

namespace gluten
{
    class icon : public element
    {
    public:
        friend class icon_button;

        icon() = default;
        icon(const std::string& displayText) : m_displayText(displayText) {}
        icon(const std::string& displayText, const ImVec2& alignment, const anchor_preset& anchorPreset)
            : element(anchorPreset), m_displayText(displayText)
        {
        }

        auto get_element_content_size(const ImVec2& parentSize) -> ImVec2 const override;

        auto set_font(fonts font) -> icon&
        {
            m_font = font;
            return *this;
        }

        /**
         * @brief Render the glyph at an exact pixel size — the size the
         *        font atlas bakes at for this draw.
         */
        auto set_pixel_size(float px) -> icon&
        {
            m_pixelSize = px;
            return *this;
        }

        /**
         * @brief Apply a smooth visual scale on top of the baked size.
         *
         * Bake happens once at set_pixel_size() (crisp, integer-rounded by
         * ImGui internally). The vertices AddText emits are then scaled
         * around the icon centre by this multiplier — fractional, sub-pixel,
         * stutter-free. Ideal for hover-grow eases where the font pipeline's
         * integer-rounded size would otherwise step visibly.
         */
        auto set_visual_scale(float scale) -> icon&
        {
            m_visualScale = scale;
            return *this;
        }

    protected:
        auto render_element(const element_render_info& renderInfo) -> bool override;
        auto pre_render_element() -> void override;
        auto post_render_element() -> void override;

    private:
        auto effective_pixel_size() const -> float;

        std::string m_displayText;
        std::optional<fonts> m_font;
        std::optional<float> m_pixelSize;
        float m_visualScale = 1.0f;
        bool m_pushedFont   = false;
    };

    class icon_button : public element
    {
    public:
        icon_button() = delete;
        icon_button(const char* buttonID, const char* icon, fonts font, button_style style = button_style::ghost);

        bool render_element(const element_render_info& renderInfo) override;
        auto get_element_content_size(const ImVec2& parentSize) -> const ImVec2 override
        {
            ImVec2 scaledParentSize = parentSize;
            scaledParentSize.x *= get_element_scale();
            scaledParentSize.y *= get_element_scale();
            return m_text.get_element_content_size(scaledParentSize);
        }

        button& get_button() { return m_button; }
        icon& get_text() { return m_text; }

        /**
         * @brief Enable a hover-driven grow animation on just the icon glyph.
         *
         * The button's hit rectangle stays fixed; only the icon eases larger
         * while the pointer is over the button and back to 1.0 when it leaves.
         * The animation id is derived from the button id given at construction,
         * so state persists across the frame-by-frame reconstruction pattern.
         *
         * @param scaleWhenHovered Target multiplier. 1.15 = grow by 15%.
         * @param rate             Smoothing rate; higher = snappier.
         */
        auto set_button_style(button_style style) -> icon_button&;
        auto set_icon_hover_grow(float scaleWhenHovered, float rate = 15.0f) -> icon_button&;

    private:
        button m_button;
        icon m_text;
        const char* m_buttonId = nullptr;
        button_style m_style   = button_style::ghost;
        std::optional<float> m_iconHoverGrowScale;
        float m_iconHoverGrowRate = 15.0f;
    };
}  // namespace gluten