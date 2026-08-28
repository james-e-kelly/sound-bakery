#pragma once

#include "gluten/elements/element.h"

namespace gluten
{
    enum class text_alignment
    {
        none,
        horizontal_center,
        vertical_center,
        center
    };

    class text : public element
    {
    public:
        friend class icon_button;

        text() = default;
        text(const std::string& displayText);
        text(const std::string& displayText, const ImVec2& alignment, const anchor_preset& anchorPreset, const text_style& style = text_style::body);
        text(const std::string& displayText, const ImVec2& alignment, const anchor_preset& anchorPreset, const text_style& style, const fonts& fontOverride);

        auto set_text(const std::string& displayText) -> text&;
        auto set_font(const fonts& font) -> text&;
        auto set_font_size(float px) -> text&;
        auto set_url(const std::string& url) -> text&;
        auto set_text_alignment(text_alignment alignment) -> void;

        auto set_text_style(text_style style) -> text&;
        auto set_text_style(text_style style, fonts fontOverride) -> text&;

        auto set_element_content_font_size(float size) -> element& override;

        auto get_element_content_size(const ImVec2& parentSize) -> ImVec2 const override;

        auto get_text() const -> const std::string& { return m_displayText; }

    protected:
        auto render_element(const element_render_info& renderInfo) -> bool override;
        auto pre_render_element() -> void override;
        auto post_render_element() -> void override;

    private:
        std::string m_displayText;
        std::string m_truncatedText;
        std::string m_url;
        std::optional<fonts> m_font;
        std::optional<float> m_fontSize;
        std::optional<ImVec4> m_textColor;
        text_alignment m_textAlignment = text_alignment::none;
    };
}  // namespace gluten