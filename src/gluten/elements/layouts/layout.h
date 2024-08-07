#pragma once

#include "gluten/elements/element.h"

namespace gluten
{
    class layout : public element
    {
    public:
        /**
         * @brief Which direction/technique to layout the children
         */
        enum class layout_type
        {
            left_to_right,
            right_to_left,
            top_to_bottom,
            bottom_to_top
        };

        layout() = default;
        layout(const layout_type& layoutType);
        layout(const layout_type& layoutType, const anchor_preset& anchorPreset);

        void set_layout_type(const layout_type& type);

        bool render_layout_element_full(element* element);
        
        bool render_layout_element_pixels(element* element, float horizontalPixels, float verticalPixels);
        bool render_layout_element_pixels_horizontal(element* element, float horizontalPixels);
        bool render_layout_element_pixels_vertical(element* element, float verticalPixels);

        bool render_layout_element_percent(element* element, float horizontalPercent, float verticalPercent);
        bool render_layout_element_percent_horizontal(element* element, float horizontalPercent);
        bool render_layout_element_percent_vertical(element* element, float verticalPercent);

        void reset_layout(const ImRect& parent);

        ImVec2 get_current_layout_pos() const
        {
            return m_currentLayoutPos.has_value() ? m_currentLayoutPos.value() : ImVec2(0, 0);
        }

        ImVec2 get_current_layout_pos_local() const;

    private:
        void setup_layout_begin(const ImRect& thisBox);

        bool render_element(const ImRect& info) override { return false; }

        bool render_layout_element_internal(const ImRect& thisBox, element* element, float horizontalPixels, float verticalPixels);

        layout_type m_layoutType = layout_type::left_to_right;

        std::optional<ImVec2> m_currentLayoutPos;

        bool m_firstLayout = true;
    };
}