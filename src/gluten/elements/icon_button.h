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

    protected:
        auto render_element(const element_render_info& renderInfo) -> bool override
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

    private:
        button m_button;
        icon m_text;
    };
}  // namespace gluten