#include "text.h"

#include "gluten/app/app.h"
#include "gluten/utils/imgui_util_structures.h"

namespace
{
    auto is_valid_character(typename const std::string::value_type& character) -> bool
    {
        return character >= 0 && character <= 255;
    }

    auto remove_last_word(std::string& str) -> void
    {
        // Trim any whitespace, then remove the characters of the word,
        // then remove any final whitespace

        while (!str.empty() && is_valid_character(str.back()) && std::isspace(str.back()))
        {
            str.pop_back();
        }

        while (!str.empty() && (!is_valid_character(str.back()) || !std::isspace(str.back())))
        {
            str.pop_back();
        }

        while (!str.empty() && is_valid_character(str.back()) && std::isspace(str.back()))
        {
            str.pop_back();
        }
    }
}

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

auto gluten::text::set_url(const std::string& url) -> text& 
{ 
    m_url = url; 
    return *this;
}

auto gluten::text::pre_render_element() -> void
{
    if (m_font.has_value())
    {
        ImGui::PushFont(gluten::app::get()->get_font(m_font.value()));
    }
}

bool gluten::text::render_element(const element_render_info& renderInfo)
{
    if (!m_displayText.empty())
    {
        ImGuiContext& context     = *GImGui;

        if (ImGuiWindow* const window = context.CurrentWindow)
        {
            if (ImDrawList* const drawList = ImGui::GetWindowDrawList())
            {
                const bool hasStrictHorizontalControl = std::abs(m_anchor.max.x - m_anchor.min.x) > 0.01f;
                const bool hasStrictVerticalControl = std::abs(m_anchor.max.y - m_anchor.min.y) > 0.01f;

                const ImVec2 textPos(window->DC.CursorPos.x, window->DC.CursorPos.y + window->DC.CurrLineTextBaseOffset);

                ImVec2 textSize;

                if (hasStrictHorizontalControl && hasStrictVerticalControl)
                {
                    // TODO: Make truncation faster or cached. Without testing, this seems like a slow approach

                    const float currentHeight = renderInfo.elementBox.GetHeight();
                    
                    m_truncatedText = m_displayText;

                    textSize = ImGui::CalcTextSize(m_truncatedText.c_str(), nullptr, false, renderInfo.elementBox.GetWidth());

                    while (textSize.y > currentHeight && textSize.y > 0.0f && !m_truncatedText.empty())
                    {
                        remove_last_word(m_truncatedText);
                        const std::string sizeTestString = m_truncatedText + "...";
                        textSize                         = ImGui::CalcTextSize(sizeTestString.c_str(), nullptr, false, renderInfo.elementBox.GetWidth());
                    }

                    if (m_displayText != m_truncatedText)
                    {
                        m_truncatedText += "...";
                    }

                    if (m_url.empty())
                    {
                        drawList->AddText(context.Font, context.FontSize, textPos, ImGui::GetColorU32(ImGuiCol_Text), m_truncatedText.c_str(), nullptr, renderInfo.elementBox.GetWidth());
                    }
                    else
                    {
                        gluten::imgui::scoped_color urlColor(ImGuiCol_TextLink, ImGui::GetColorU32(ImGuiCol_Text));   
                        ImGui::TextLinkOpenURL(m_truncatedText.c_str(), m_url.c_str());
                    }
                }
                else
                {
                    textSize = ImGui::CalcTextSize(m_displayText.c_str(), nullptr, false, renderInfo.elementBox.GetWidth());

                    if (m_url.empty())
                    {
                        drawList->AddText(context.Font, context.FontSize, textPos, ImGui::GetColorU32(ImGuiCol_Text), m_displayText.c_str(), nullptr, renderInfo.elementBox.GetWidth());
                    }
                    else
                    {
                        ImGui::TextLinkOpenURL(m_displayText.c_str(), m_url.c_str());
                    }
                }

                /*if (!m_url.empty())
                {
                    bool hovered       = false;
                    const bool pressed = ImGui::ButtonBehavior(parent, window->GetID(m_displayText.c_str()), &hovered, nullptr);

                    if (pressed)
                    {
                        if (context.PlatformIO.Platform_OpenInShellFn != NULL)
                        {
                            context.PlatformIO.Platform_OpenInShellFn(&context, m_url.c_str());
                        }
                    }

                    ImGui::SetItemTooltip(ImGui::LocalizeGetMsg(ImGuiLocKey_OpenLink_s), m_url.c_str());

                    if (hovered)
                    {
                        const float lineY = textPos.y + context.FontBaked->Ascent * (context.FontSize / context.FontBaked->Size) + (context.Style.FramePadding.y * 0.5f);
                        drawList->AddLine(ImVec2(parent.Min.x, lineY), ImVec2(parent.Min.x + textSize.x, lineY), ImGui::GetColorU32(ImGuiCol_TextLink));
                    }
                }*/
            }
        }
    }

    return false;
}

auto gluten::text::post_render_element() -> void
{
    if (m_font.has_value())
    {
        ImGui::PopFont();
    }
}

auto gluten::text::get_element_content_size(const ImVec2& parentSize) -> ImVec2 const { return ImGui::CalcTextSize(m_displayText.c_str()); }