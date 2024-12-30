#pragma once

#include "gluten/elements/element.h"

namespace gluten
{
    class text : public element
    {
    public:
        text() = default;
        text(const std::string& displayText);
        text(const std::string& displayText, const ImVec2& alignment, const anchor_preset& anchorPreset);

        void set_text(const std::string& displayText);
        void set_font(const fonts& font);

        auto get_element_content_size() -> ImVec2 const override;

    protected:
        auto render_element(const ImRect& parent) -> bool override;
        auto pre_render_element() -> void override;

    private:
        std::string m_displayText;
        std::optional<fonts> m_font;
    };
}  // namespace gluten