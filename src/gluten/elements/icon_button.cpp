#include "icon_button.h"

#include "gluten/app/app.h"
#include "gluten/subsystems/animation_subsystem.h"
#include "gluten/theme/theme.h"


gluten::icon_button::icon_button(const char* buttonID, const char* icon, fonts font, button_style style)
    : m_button(buttonID, true, anchor_preset::left_top), m_text(icon), m_buttonId(buttonID), m_style(style)
{
    set_element_anchor_preset(anchor_preset::stretch_full);
    // Auto-key animation state off the button id so hover-fill fade and
    // hover-grow both work out of the box for every icon_button.
    set_animation_id(buttonID);

    m_button.set_element_anchor_preset(anchor_preset::stretch_full);

    m_text
        .set_font(font)
        .set_element_anchor_preset(anchor_preset::center_middle);
}

auto gluten::icon_button::set_button_style(button_style style) -> icon_button&
{
    m_style = style;
    return *this;
}

auto gluten::icon_button::set_icon_hover_grow(float scaleWhenHovered, float rate) -> icon_button&
{
    m_iconHoverGrowScale = scaleWhenHovered;
    m_iconHoverGrowRate  = rate;
    return *this;
}

bool gluten::icon_button::render_element(const element_render_info& renderInfo)
{
    ImGui::BeginGroup();

    ImGui::PushStyleColor(ImGuiCol_Text, gluten::theme::icon_color_for(m_style));

    const float baseIconPx = gluten::theme::textSizeBody * get_element_scale();

    // Animated visual multiplier via the subsystem. Applied as a post-render
    // vertex scale (not as a font size) so the ease is sub-pixel-smooth
    // instead of stepping at ImGui's integer-rounded font sizes.
    float visualScale = 1.0f;
    if (m_iconHoverGrowScale.has_value() && m_buttonId != nullptr)
    {
        const bool hovered = ImGui::IsMouseHoveringRect(renderInfo.elementBox.Min, renderInfo.elementBox.Max);
        const ImGuiID id   = ImGui::GetID(m_buttonId);
        const float target = hovered ? m_iconHoverGrowScale.value() : 1.0f;
        visualScale        = animation_subsystem::animate(id, 1.0f, target, m_iconHoverGrowRate);
    }

    // Button hit-rect uses the un-animated icon size so it stays fixed while
    // the glyph grows on hover.
    m_text.set_pixel_size(baseIconPx);
    m_text.set_visual_scale(1.0f);
    m_button.set_element_min_size(m_text.get_element_content_size(renderInfo.elementBox.GetSize()));
    const bool buttonActivated = m_button.render(renderInfo.elementBox);

    // Now animate the glyph itself.
    m_text.set_visual_scale(visualScale);
    m_text.render(renderInfo.elementBox);
    ImGui::PopStyleColor();
    ImGui::EndGroup();
    return buttonActivated;
}

auto gluten::icon::effective_pixel_size() const -> float
{
    if (m_pixelSize.has_value())
    {
        return m_pixelSize.value();
    }
    return gluten::theme::textSizeBody * get_element_scale();
}

auto gluten::icon::get_element_content_size(const ImVec2& parentSize) -> ImVec2 const
{
    if (m_pushedFont)
    {
        const float size = GImGui->FontSize;
        return ImVec2(size, size);
    }
    const float size = effective_pixel_size();
    return ImVec2(size, size);
}

auto gluten::icon::pre_render_element() -> void
{
    if (m_font.has_value())
    {
        ImGui::PushFont(gluten::app::get()->get_font(m_font.value()), effective_pixel_size());
        m_pushedFont = true;
    }
}

auto gluten::icon::post_render_element() -> void
{
    if (m_pushedFont)
    {
        ImGui::PopFont();
        m_pushedFont = false;
    }
}

auto gluten::icon::render_element(const element_render_info& renderInfo) -> bool
{
    if (m_displayText.empty())
    {
        return false;
    }

    ImGuiContext& context      = *GImGui;
    ImGuiWindow* const window  = context.CurrentWindow;
    ImDrawList* const drawList = window ? ImGui::GetWindowDrawList() : nullptr;

    if (!window || !drawList)
    {
        return false;
    }

    const ImVec2 boxSize      = renderInfo.elementBox.GetSize();
    const ImVec2 renderedSize = ImGui::CalcTextSize(m_displayText.c_str());

    float glyphVisualCenterY = renderedSize.y * 0.5f;
    if (context.FontBaked)
    {
        unsigned int codepoint = 0;
        ImTextCharFromUtf8(&codepoint, m_displayText.c_str(), m_displayText.c_str() + m_displayText.size());
        const ImFontGlyph* glyph = context.FontBaked->FindGlyphNoFallback((ImWchar)codepoint);
        if (glyph)
        {
            const float scale = context.FontSize / context.FontBaked->Size;
            glyphVisualCenterY = (glyph->Y0 + glyph->Y1) * scale * 0.5f;
        }
    }

    const ImVec2 cursorPos = window->DC.CursorPos;
    const ImVec2 textPos(cursorPos.x + (boxSize.x - renderedSize.x) * 0.5f,
                         cursorPos.y + boxSize.y * 0.5f - glyphVisualCenterY);

    if (m_visualScale == 1.0f)
    {
        drawList->AddText(context.Font, context.FontSize, textPos, ImGui::GetColorU32(ImGuiCol_Text), m_displayText.c_str());
        return false;
    }

    // Post-render vertex scale: capture the range AddText appends, then
    // rescale those vertices around the element centre. This bypasses
    // ImGui's integer-rounded font sizing entirely, so the ease is smooth
    // at any fractional multiplier.
    const int vtxStart = drawList->VtxBuffer.Size;
    drawList->AddText(context.Font, context.FontSize, textPos, ImGui::GetColorU32(ImGuiCol_Text), m_displayText.c_str());
    const int vtxEnd = drawList->VtxBuffer.Size;

    const ImVec2 pivot(cursorPos.x + boxSize.x * 0.5f, cursorPos.y + boxSize.y * 0.5f);
    const float s = m_visualScale;
    for (int i = vtxStart; i < vtxEnd; ++i)
    {
        ImDrawVert& v = drawList->VtxBuffer[i];
        v.pos.x       = pivot.x + (v.pos.x - pivot.x) * s;
        v.pos.y       = pivot.y + (v.pos.y - pivot.y) * s;
    }

    return false;
}
