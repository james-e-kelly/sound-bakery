#include "text.h"

#include "gluten/app/app.h"
#include "gluten/theme/theme.h"
#include "gluten/utils/imgui_util_structures.h"

namespace
{
    auto is_valid_character(const std::string::value_type& character) -> bool
    {
        return character >= 0 && character <= 255;
    }

    auto remove_last_word(std::string& str) -> void
    {
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

    auto offset_vertices(ImDrawList* drawList, int vtxStart, ImVec2 offset) -> void
    {
        const int vtxEnd = drawList->VtxBuffer.Size;
        for (int i = vtxStart; i < vtxEnd; ++i)
        {
            ImDrawVert& v = drawList->VtxBuffer[i];
            v.pos.x += offset.x;
            v.pos.y += offset.y;
        }
    }
}  // namespace

gluten::text::text(const std::string& displayText) : m_displayText(displayText) {}

gluten::text::text(const std::string& displayText, const ImVec2& alignment, const anchor_preset& anchorPreset, const text_style& style)
    : element(anchorPreset), m_displayText(displayText)
{
    m_alignment = alignment;
    set_text_style(style);
}

gluten::text::text(const std::string& displayText, const ImVec2& alignment, const anchor_preset& anchorPreset, const text_style& style, const fonts& fontOverride)
    : text(displayText, alignment, anchorPreset, style)
{
    set_font(fontOverride);
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

auto gluten::text::set_text_alignment(text_alignment alignment) -> void
{
    m_textAlignment = alignment;
}

auto gluten::text::set_font_size(float px) -> text&
{
    m_fontSize = px;
    return *this;
}

auto gluten::text::set_text_style(text_style style) -> text&
{
    set_font(gluten::theme::text_font_for(style));
    m_fontSize  = gluten::theme::text_size_for(style);
    m_textColor = gluten::theme::text_color_for(style);

    return *this;
}

auto gluten::text::set_text_style(text_style style, fonts fontOverride) -> text&
{
    set_font(fontOverride);
    m_fontSize  = gluten::theme::text_size_for(style);
    m_textColor = gluten::theme::text_color_for(style);
    return *this;
}

auto gluten::text::set_element_content_font_size(float size) -> element&
{
    m_fontSize = size;
    return *this;
}

auto gluten::text::pre_render_element() -> void
{
    if (m_font.has_value() && m_fontSize.has_value())
    {
        ImGui::PushFont(gluten::app::get()->get_font(m_font.value()), m_fontSize.value());
    }
    else if (m_font.has_value())
    {
        ImGui::PushFont(gluten::app::get()->get_font(m_font.value()));
    }
    else if (m_fontSize.has_value())
    {
        ImGui::PushFont(nullptr, m_fontSize.value());
    }

    if (m_textColor.has_value())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, m_textColor.value());
    }
}

bool gluten::text::render_element(const element_render_info& renderInfo)
{
    if (!m_displayText.empty())
    {
        ImGuiContext& context = *GImGui;

        if (ImGuiWindow* const window = context.CurrentWindow)
        {
            if (ImDrawList* const drawList = ImGui::GetWindowDrawList())
            {
                const bool hasConstrainedWidth        = std::abs(m_anchor.max.x - m_anchor.min.x) > 0.01f;
                const bool hasStrictHorizontalControl = hasConstrainedWidth && (m_textAlignment != text_alignment::horizontal_center && m_textAlignment != text_alignment::center);
                const bool hasStrictVerticalControl   = std::abs(m_anchor.max.y - m_anchor.min.y) > 0.01f && (m_textAlignment != text_alignment::vertical_center && m_textAlignment != text_alignment::center);
                const float wrapWidth                 = hasConstrainedWidth ? renderInfo.elementBox.GetWidth() : 0.0f;

                const ImVec2 rawTextPos(window->DC.CursorPos.x, window->DC.CursorPos.y + window->DC.CurrLineTextBaseOffset);
                const ImVec2 snappedTextPos(std::floor(rawTextPos.x), std::floor(rawTextPos.y));
                const ImVec2 subPixelOffset(rawTextPos.x - snappedTextPos.x, rawTextPos.y - snappedTextPos.y);

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
                        const int vtxStart = drawList->VtxBuffer.Size;
                        drawList->AddText(context.Font, context.FontSize, snappedTextPos, ImGui::GetColorU32(ImGuiCol_Text), m_truncatedText.c_str(), nullptr, renderInfo.elementBox.GetWidth());
                        offset_vertices(drawList, vtxStart, subPixelOffset);
                    }
                    else
                    {
                        gluten::imgui::scoped_color urlColor(ImGuiCol_TextLink, ImGui::GetColorU32(ImGuiCol_Text));
                        ImGui::TextLinkOpenURL(m_truncatedText.c_str(), m_url.c_str());
                    }
                }
                else
                {
                    textSize = ImGui::CalcTextSize(m_displayText.c_str(), nullptr, false, wrapWidth);

                    if (m_url.empty())
                    {
                        ImVec2 textPosWithAlignment = snappedTextPos;

                        switch (m_textAlignment)
                        {
                            case gluten::text_alignment::horizontal_center:
                                textPosWithAlignment.x -= std::floor(textSize.x / 2.0f);
                                break;
                            case gluten::text_alignment::vertical_center:
                                textPosWithAlignment.y -= std::floor(context.FontSize / 2.0f);
                                break;
                            case gluten::text_alignment::center:
                                textPosWithAlignment.x -= std::floor(textSize.x / 2.0f);
                                textPosWithAlignment.y -= std::floor(context.FontSize / 2.0f);
                                break;
                            default:
                                break;
                        }

                        const int vtxStart = drawList->VtxBuffer.Size;
                        drawList->AddText(context.Font, context.FontSize, textPosWithAlignment, ImGui::GetColorU32(ImGuiCol_Text), m_displayText.c_str(), nullptr, wrapWidth);
                        offset_vertices(drawList, vtxStart, subPixelOffset);
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
    if (m_textColor.has_value())
    {
        ImGui::PopStyleColor();
    }

    if (m_font.has_value() || m_fontSize.has_value())
    {
        ImGui::PopFont();
    }
}

auto gluten::text::get_element_content_size(const ImVec2& parentSize) -> ImVec2 const
{
    const bool hasConstrainedWidth = std::abs(m_anchor.max.x - m_anchor.min.x) > 0.01f;
    const float wrapWidth          = hasConstrainedWidth && parentSize.x > 0.0f
        ? parentSize.x * (m_anchor.max.x - m_anchor.min.x)
        : 0.0f;

    return ImGui::CalcTextSize(m_displayText.c_str(), nullptr, false, wrapWidth);
}