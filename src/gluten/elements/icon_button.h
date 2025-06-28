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

        auto get_element_content_size() -> ImVec2 const override;

        auto set_font(fonts font) -> icon& 
        { 
            m_font = font;
            return *this;
        }

    protected:
        auto render_element(const ImRect& parent) -> bool override
        {
            // Magic numbers!
            // Needed to almost-perfectly align the icons
            static constexpr float xOffset = 1.325f;
            static constexpr float yOffset = 0.325f;

            if (!m_displayText.empty())
            {
                ImGuiContext& context = *GImGui;

                if (ImGuiWindow* const window = context.CurrentWindow)
                {
                    if (ImDrawList* const drawList = ImGui::GetWindowDrawList())
                    {
                        const float scale = get_element_scale();

                        const float scaledXPushback = xOffset * scale;
                        const float scaledYPushback = yOffset * scale;

                        const ImVec2 textPos(window->DC.CursorPos.x - scaledXPushback, window->DC.CursorPos.y + window->DC.CurrLineTextBaseOffset - scaledYPushback);

                        assert(window->DC.CursorPos.x - scaledXPushback == textPos.x);

                        drawList->AddText(context.Font, context.FontSize, textPos, ImGui::GetColorU32(ImGuiCol_Text), m_displayText.c_str());
                    }
                }
            }

            if (m_font.has_value())
            {
                ImGui::PopFont();
            }
            return false;
        }

        auto pre_render_element() -> void override;

    private:
        std::string m_displayText;
        std::optional<fonts> m_font;
    };

    class icon_button : public element
    {
    public:
        icon_button() = delete;
        icon_button(const char* buttonID, const char* icon, fonts font);

        bool render_element(const ImRect& parent) override;

        button& get_button() { return m_button; }
        icon& get_text() { return m_text; }

        auto set_element_min_size(const ImVec2& minSize) -> element&;
        auto set_element_max_size(const ImVec2& maxSize) -> element&;
        auto set_element_content_scale(float scale) -> element&;
        auto set_icon_alignment(const ImVec2& alignment) -> icon_button&;
        auto set_button_alignment(const ImVec2& alignment) -> icon_button&;
        auto set_button_scale(float scale) -> icon_button&;
        auto set_button_border(float borderSize, float borderRounding) -> icon_button&;

        // Override colors
        // Only the button should get hover colors
        // Never this element itself
        auto set_element_active(bool active) -> element& override;
        auto set_element_background_color(ImU32 color) -> element& override;
        auto set_element_background_color(ImVec4 color) -> element& override;
        auto set_element_hover_color(ImU32 color) -> element& override;
        auto set_element_hover_color(ImVec4 color) -> element& override;
        auto set_element_active_color(ImU32 color) -> element& override;
        auto set_element_active_color(ImVec4 color) -> element& override;

    private:
        button m_button;
        icon m_text;
    };
}  // namespace gluten