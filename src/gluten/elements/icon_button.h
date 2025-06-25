#pragma once

#include "gluten/elements/button.h"
#include "gluten/elements/text.h"

namespace gluten
{
    class icon_button : public element
    {
    public:
        icon_button() = delete;
        icon_button(const char* buttonID, const char* icon, fonts font);

        bool render_element(const ImRect& parent) override;

        button& get_button() { return m_button; }
        text& get_text() { return m_text; }

        void set_element_min_size(const ImVec2& minSize) override;
        void set_element_max_size(const ImVec2& maxSize) override;

    private:
        button m_button;
        text m_text;
    };
}  // namespace gluten