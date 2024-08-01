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

        void set_layout_type(const layout_type& type);
        
        /**
         * @brief Render an element and give it a portion of this layout's area.
         */
        bool render_layout_element(element* element, float horizontalPercent, float verticalPercent);

    private:
        bool render_element(const box& info) override { return false; }

        layout_type m_layoutType = layout_type::left_to_right;

        std::optional<ImVec2> m_currentLayoutPos;
    };
}