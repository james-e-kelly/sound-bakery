#include "text.h"

#include "gluten/app/app.h"

gluten::text::text(const std::string& displayText) : m_displayText(displayText) {}

gluten::text::text(const std::string& displayText, const ImVec2& alignment, const anchor_preset& anchorPreset)
    : element(anchorPreset), m_displayText(displayText)
{
    m_alignment = alignment;
}

auto gluten::text::set_text(const std::string& displayText) -> text& 
{ 
    m_displayText = displayText; 
    return *this;
}

auto gluten::text::set_font(const fonts& font) -> text& 
{ 
    m_font = font; 
    return *this;
}

auto gluten::text::pre_render_element() -> void
{
    if (m_font.has_value())
    {
        ImGui::PushFont(gluten::app::get()->get_font(m_font.value()));
    }
}

bool gluten::text::render_element(const ImRect& parent)
{
    if (!m_displayText.empty())
    {
        ImGuiContext& context     = *GImGui;

        if (ImGuiWindow* const window = context.CurrentWindow)
        {
            if (ImDrawList* const drawList = ImGui::GetWindowDrawList())
            {
                const ImVec2 textPos(window->DC.CursorPos.x, window->DC.CursorPos.y + window->DC.CurrLineTextBaseOffset);
                drawList->AddText(context.Font, context.FontSize, textPos, ImGui::GetColorU32(ImGuiCol_Text), m_displayText.c_str(), nullptr, parent.GetWidth());
            }
        }
    }

    if (m_font.has_value())
    {
        ImGui::PopFont();
    }
    return false;
}

auto gluten::text::get_element_content_size() -> ImVec2 const { return ImGui::CalcTextSize(m_displayText.c_str()); }