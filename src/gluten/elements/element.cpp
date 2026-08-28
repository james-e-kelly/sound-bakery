#include "element.h"

#include "gluten/subsystems/animation_subsystem.h"
#include "gluten/theme/carbon/carbon_colors.h"
#include "gluten/utils/imgui_rect_operators.h"

void gluten::element::anchor_info::set_achor_from_preset(const anchor_preset& preset)
{
    anchorPreset = preset;

    switch (preset)
    {
        case anchor_preset::left_top:
            min = ImVec2(0, 0);
            max = ImVec2(0, 0);
            break;
        case anchor_preset::center_top:
            min = ImVec2(0.5f, 0);
            max = ImVec2(0.5f, 0);
            break;
        case anchor_preset::right_top:
            min = ImVec2(1, 0);
            max = ImVec2(1, 0);
            break;

        case anchor_preset::left_middle:
            min = ImVec2(0, 0.5f);
            max = ImVec2(0, 0.5f);
            break;
        case anchor_preset::center_middle:
            min = ImVec2(0.5f, 0.5f);
            max = ImVec2(0.5f, 0.5f);
            break;
        case anchor_preset::right_middle:
            min = ImVec2(1, 0.5f);
            max = ImVec2(1, 0.5f);
            break;

        case anchor_preset::left_bottom:
            min = ImVec2(0, 1);
            max = ImVec2(0, 1);
            break;
        case anchor_preset::center_bottom:
            min = ImVec2(0.5f, 1);
            max = ImVec2(0.5f, 1);
            break;
        case anchor_preset::right_bottom:
            min = ImVec2(1, 1);
            max = ImVec2(1, 1);
            break;

        case anchor_preset::stretch_left:
            min = ImVec2(0, 0);
            max = ImVec2(0, 1);
            break;
        case anchor_preset::stretch_center:
            min = ImVec2(0.5f, 0);
            max = ImVec2(0.5f, 1);
            break;
        case anchor_preset::stretch_right:
            min = ImVec2(1, 0);
            max = ImVec2(1, 1);
            break;

        case anchor_preset::stretch_top:
            min = ImVec2(0, 0);
            max = ImVec2(1, 0);
            break;
        case anchor_preset::stretch_middle:
            min = ImVec2(0, 0.5f);
            max = ImVec2(1, 0.5f);
            break;
        case anchor_preset::stretch_bottom:
            min = ImVec2(0, 1);
            max = ImVec2(1, 1);
            break;

        case anchor_preset::stretch_full:
            min = ImVec2(0, 0);
            max = ImVec2(1, 1);
            break;
    }
}

auto gluten::element::anchor_info::set_min_offset(ImVec2 offset) -> anchor_info&
{
    minOffset = offset;
    return *this;
}

auto gluten::element::anchor_info::set_max_offset(ImVec2 offset) -> anchor_info&
{
    maxOffset = offset;
    return *this;
}

gluten::element::element()
{
    refresh_element();
}

gluten::element::element(const anchor_preset& anchorPreset) 
{ 
    m_anchor.set_achor_from_preset(anchorPreset); 
    refresh_element();
}

gluten::element::~element()
{
    if (ImGui::GetCurrentContext())
    {
        ImGui::SetWindowFontScale(1.0f);
    }
}

auto gluten::element::set_element_content_font_size(float size) -> element&
{
    const float currentFontSize = ImGui::GetFontSize();

    const float scale = size / currentFontSize;

    set_element_content_scale(scale);

    return *this;
}

auto gluten::element::set_element_content_scale(float scale) -> element&
{
    m_contentScale = scale;
    return *this;
}

auto gluten::element::set_element_scale(float scale) -> element&
{
    m_scale = scale;
    return *this;
}

auto gluten::element::has_element_scale() const -> bool { return m_contentScale.has_value(); }

auto gluten::element::get_element_scale() const -> float { return has_element_scale() ? m_contentScale.value() : 1.0f; }

auto gluten::element::set_element_background_color(ImU32 color) -> element&
{
    m_backgroundColor = color;
    return *this;
}

auto gluten::element::set_element_background_color(ImVec4 color) -> element&
{
    set_element_background_color(ImGui::ColorConvertFloat4ToU32(color));
    return *this;
}

auto gluten::element::set_element_hover_color(ImU32 color) -> element&
{
    m_hoverColor = color;
    return *this;
}

auto gluten::element::set_element_hover_color(ImVec4 color) -> element&
{
    set_element_hover_color(ImGui::ColorConvertFloat4ToU32(color));
    return *this;
}

auto gluten::element::set_element_active_color(ImU32 color) -> element&
{
    m_activeColor = color;
    return *this;
}

auto gluten::element::set_element_active_color(ImVec4 color) -> element&
{
    set_element_active_color(ImGui::ColorConvertFloat4ToU32(color));
    return *this;
}

auto gluten::element::set_element_active(bool active) -> element&
{
    m_active = active;
    return *this;
}

auto gluten::element::set_element_padding(const ImVec2& padding) -> element&
{
    m_padding = ImVec4(padding.x, padding.y, padding.x, padding.y);
    return *this;
}

auto gluten::element::set_element_padding(const ImVec4& padding) -> element&
{
    m_padding = padding;
    return *this;
}

auto gluten::element::set_element_window_padding() -> element&
{
    const ImVec2& p = ImGui::GetStyle().WindowPadding;
    m_padding = ImVec4(p.x, p.y, p.x, p.y);
    return *this;
}

auto gluten::element::set_element_frame_padding() -> element&
{
    const ImVec2& p = ImGui::GetStyle().FramePadding;
    m_padding = ImVec4(p.x, p.y, p.x, p.y);
    return *this;
}

auto gluten::element::set_element_inner_padding(const ImVec2& padding) -> element&
{
    m_innerPadding = ImVec4(padding.x, padding.y, padding.x, padding.y);
    return *this;
}

auto gluten::element::set_element_inner_padding(const ImVec4& padding) -> element&
{
    m_innerPadding = padding;
    return *this;
}

auto gluten::element::set_element_inner_padding(float padding) -> element&
{
    m_innerPadding = ImVec4(padding, padding, padding, padding);
    return *this;
}

gluten::element::anchor_info& gluten::element::get_element_anchor() { return m_anchor; }

ImRect gluten::element::get_element_rect() const
{
    if (!m_currentRect.has_value())
    {
        m_currentRect = ImGui::GetCurrentWindow()->WorkRect;
    }
    return m_currentRect.value();
}

ImRect gluten::element::get_element_content_rect() const
{
    const ImRect outer = get_element_rect();
    return ImRect(outer.Min.x + m_innerPadding.x, outer.Min.y + m_innerPadding.y,
                  outer.Max.x - m_innerPadding.z, outer.Max.y - m_innerPadding.w);
}

ImRect gluten::element::get_element_rect_local() const
{
    ImRect elementRect = get_element_rect();
    elementRect.Translate(-ImGui::GetWindowPos());
    return elementRect;
}

auto gluten::element::get_element_is_hovered() const -> bool
{
    ImRect elementRect = get_element_rect();
    return ImGui::IsMouseHoveringRect(elementRect.Min, elementRect.Max);
}

bool gluten::element::render(const ImRect& parent)
{
    ZoneScoped;

    pre_render_element();

    // Compute the element's rect at its unscaled state first so hover
    // detection is stable (a growing rect would drag hover in/out as it grows).
    const std::pair<ImRect, ImRect> baseElementBoxes = get_element_box_from_parent(parent, m_minSize, get_element_content_size(parent.GetSize()), m_alignment, m_padding, m_anchor);
    const ImRect baseElementBox                      = (baseElementBoxes.first * m_scale) + m_translation;
    const bool hovered                               = ImGui::IsMouseHoveringRect(baseElementBox.Min, baseElementBox.Max);

    // Hover-grow: ease content scale toward target-when-hovered, else 1.0.
    // Persistent per-element via ImGui storage keyed by the animation id.
    const float baseContentScale = m_contentScale.value_or(1.0f);
    float appliedContentScale    = baseContentScale;
    if (m_hoverGrowScale.has_value() && !m_animationId.empty())
    {
        const ImGuiID id       = ImGui::GetID(m_animationId.c_str());
        const float target     = hovered ? m_hoverGrowScale.value() : 1.0f;
        const float multiplier = animation_subsystem::animate(id, 1.0f, target, m_hoverGrowRate);
        appliedContentScale    = baseContentScale * multiplier;
    }

    if (m_contentScale.has_value() || appliedContentScale != 1.0f)
    {
        ImGui::SetWindowFontScale(appliedContentScale);
    }
    else
    {
        ImGui::SetWindowFontScale(1.0f);
    }

    // Recompute after the (potentially scaled) content size is known, so
    // children that size themselves from get_element_content_size() see the
    // enlarged size this frame.
    const std::pair<ImRect, ImRect> elementBoxes = get_element_box_from_parent(parent, m_minSize, get_element_content_size(parent.GetSize()), m_alignment, m_padding, m_anchor);
    const ImRect elementBox                      = (elementBoxes.first * m_scale) + m_translation;
    const ImRect elementBoxNoPadding             = (elementBoxes.second * m_scale) + m_translation;
    m_currentRect                                = elementBox;

    ImDrawList* const windowDrawList = ImGui::GetWindowDrawList();

    ImGui::SetCursorScreenPos(elementBoxNoPadding.Min);
    ImGui::Dummy(elementBoxNoPadding.GetSize());
    ImGui::SetCursorScreenPos(elementBox.Min);

    const bool isItemVisible = ImGui::IsRectVisible(elementBox.Min, elementBox.Max);

    if (windowDrawList && m_borderSize.has_value())
    {
        const float borderSize = m_borderSize.value();

        if (borderSize > 0.0f && !!(m_borderSides & border_sides::all))
        {
            const ImU32 borderColor = m_borderColor.value_or(ImGui::GetColorU32(ImGuiCol_Border));

            if (m_borderSides == border_sides::all)
            {
                const float borderRounding = m_borderRounding.value_or(ImGui::GetStyle().FrameRounding);

                ImRect borderRect = elementBox;
                borderRect.Expand(borderSize);

                windowDrawList->AddRectFilled(borderRect.GetTL(), borderRect.GetBR(), borderColor, borderRounding);
            }
            else
            {
                const float half = borderSize * 0.5f;

                if (!!(m_borderSides & border_sides::top))
                {
                    windowDrawList->AddLine(ImVec2(elementBox.Min.x, elementBox.Min.y - half), ImVec2(elementBox.Max.x, elementBox.Min.y - half), borderColor, borderSize);
                }

                if (!!(m_borderSides & border_sides::bottom))
                {
                    windowDrawList->AddLine(ImVec2(elementBox.Min.x, elementBox.Max.y + half), ImVec2(elementBox.Max.x, elementBox.Max.y + half), borderColor, borderSize);
                }

                if (!!(m_borderSides & border_sides::left))
                {
                    windowDrawList->AddLine(ImVec2(elementBox.Min.x - half, elementBox.Min.y), ImVec2(elementBox.Min.x - half, elementBox.Max.y), borderColor, borderSize);
                }

                if (!!(m_borderSides & border_sides::right))
                {
                    windowDrawList->AddLine(ImVec2(elementBox.Max.x + half, elementBox.Min.y), ImVec2(elementBox.Max.x + half, elementBox.Max.y), borderColor, borderSize);
                }
            }
        }
    }

    // Fill target — same precedence as before (background → active → hover),
    // but split into RGB and alpha so we can ease OPACITY only when there's
    // no natural "resting colour" to fall back to. Easing all four channels
    // toward (0,0,0,0) makes the button briefly darken on the way through,
    // reading as a flash; keeping RGB pinned to the destination colour and
    // just moving alpha gives a clean opacity fade.
    std::optional<ImU32> destRGB;
    bool destVisible = false;

    if (m_active && !hovered && m_activeColor.has_value())
    {
        destRGB     = m_activeColor.value();
        destVisible = true;
    }
    else if (hovered && m_hoverColor.has_value())
    {
        destRGB     = m_hoverColor.value();
        destVisible = true;
    }
    else if (m_backgroundColor.has_value())
    {
        destRGB     = m_backgroundColor.value();
        destVisible = true;
    }
    else
    {
        // No background — pin RGB to whichever state colour we'd fade back to
        // so the fade-out is pure opacity, not a crossfade through the void.
        if (m_hoverColor.has_value())      destRGB = m_hoverColor.value();
        else if (m_activeColor.has_value()) destRGB = m_activeColor.value();
        destVisible = false;
    }

    if (destRGB.has_value())
    {
        ImVec4 targetFillVec = ImGui::ColorConvertU32ToFloat4(destRGB.value());
        targetFillVec.w      = destVisible ? targetFillVec.w : 0.0f;

        ImVec4 initialFillVec;
        if (m_backgroundColor.has_value())
        {
            initialFillVec = ImGui::ColorConvertU32ToFloat4(m_backgroundColor.value());
        }
        else
        {
            initialFillVec   = targetFillVec;
            initialFillVec.w = 0.0f;
        }

        const bool canAnimateFill = !m_animationId.empty();
        const ImVec4 fillVec = canAnimateFill
            ? animation_subsystem::animate_color(ImGui::GetID((m_animationId + "##fill").c_str()), initialFillVec, targetFillVec)
            : targetFillVec;

        if (windowDrawList && fillVec.w > 0.0f)
        {
            windowDrawList->AddRectFilled(elementBox.Min, elementBox.Max, ImGui::ColorConvertFloat4ToU32(fillVec), m_elementRounding, m_drawFlags);
        }
    }

    const ImRect contentBox = ImRect(elementBox.Min.x + m_innerPadding.x, elementBox.Min.y + m_innerPadding.y,
                                     elementBox.Max.x - m_innerPadding.z, elementBox.Max.y - m_innerPadding.w);

    element_render_info renderInfo;
    renderInfo.elementBox = contentBox;
    renderInfo.isVisible  = isItemVisible;

    const bool activated = render_element(renderInfo);

    if (s_debug)
    {
        windowDrawList->AddRect(elementBoxNoPadding.Min, elementBoxNoPadding.Max, ImGui::ColorConvertFloat4ToU32(gluten::theme::red50), m_elementRounding);
        windowDrawList->AddRect(elementBox.Min, elementBox.Max, ImGui::ColorConvertFloat4ToU32(gluten::theme::color_with_multiplied_value(gluten::theme::red50, 0.5f)), m_elementRounding);
    }

    if (s_debugVertical)
    {
        ImVec2 leftMiddle = elementBox.GetTL();
        leftMiddle.y += elementBox.GetSize().y / 2.0f;

        ImVec2 rightMiddle = leftMiddle;
        rightMiddle.x += elementBox.GetSize().x;

        windowDrawList->AddLine(leftMiddle, rightMiddle, ImGui::ColorConvertFloat4ToU32(gluten::theme::orange50));
    }

    if (s_debugHorizontal)
    {
        ImVec2 topCenter = elementBox.GetTL();
        topCenter.x += elementBox.GetSize().x / 2.0f;

        ImVec2 bottomCenter = topCenter;
        bottomCenter.y += elementBox.GetSize().y;

        windowDrawList->AddLine(bottomCenter, topCenter, ImGui::ColorConvertFloat4ToU32(gluten::theme::orange50));
    }

    post_render_element();

    return activated;
}

bool gluten::element::render_window()
{
    const ImGuiWindow* window = ImGui::GetCurrentWindow();
    return render(window->InnerRect);
}

bool gluten::element::render_cursor()
{
    const ImVec2 cursorPos   = ImGui::GetCursorScreenPos();
    const ImVec2 elementSize = get_element_content_size();
    const ImRect rect(cursorPos, cursorPos + elementSize);
    return render(rect);
}

auto gluten::element::set_element_alignment(const ImVec2& alignment) -> element&
{
    m_alignment = alignment;
    return *this;
}

auto gluten::element::set_element_anchor_preset(const anchor_preset& preset) -> element&
{
    get_element_anchor().set_achor_from_preset(preset);
    m_alignment = get_element_anchor().min;
    return *this;
}

auto gluten::element::set_element_min_size(const ImVec2& minSize) -> element&
{
    m_minSize = minSize;
    return *this;
}

auto gluten::element::set_element_max_size(const ImVec2& maxSize) -> element&
{
    m_maxSize = maxSize;
    return *this;
}

auto gluten::element::set_element_translation(const ImVec2& translation) -> element&
{
    m_translation = translation;
    return *this;
}

auto gluten::element::set_element_border(float borderSize, float borderRounding) -> element&
{
    m_borderSize     = borderSize;
    m_borderRounding = borderRounding;
    return *this;
}

auto gluten::element::set_element_border_sides(border_sides sides) -> element&
{
    m_borderSides = sides;
    if (!m_borderSize.has_value())
    {
        m_borderSize = 1.0f;
    }
    return *this;
}

auto gluten::element::set_element_border_color(ImU32 color) -> element&
{
    m_borderColor = color;
    return *this;
}

auto gluten::element::set_element_border_color(ImVec4 color) -> element&
{
    set_element_border_color(ImGui::ColorConvertFloat4ToU32(color));
    return *this;
}

auto gluten::element::set_element_rounding(float rounding) -> element&
{
    m_elementRounding = rounding;
    return *this;
}

auto gluten::element::set_element_rounding_flags(ImDrawFlags flags) -> element&
{
    m_drawFlags = flags;
    return *this;
}

auto gluten::element::set_animation_id(std::string_view id) -> element&
{
    m_animationId.assign(id);
    return *this;
}

auto gluten::element::set_hover_grow(float scaleWhenHovered, float rate) -> element&
{
    m_hoverGrowScale = scaleWhenHovered;
    m_hoverGrowRate  = rate;
    return *this;
}

ImVec2 gluten::element::get_anchor_start_position(const ImVec2& containerPosition,
                                                  const ImVec2& containerSize,
                                                  const anchor_info& anchor)
{
    const float xPositionWithOffset = containerPosition.x + anchor.minOffset.x;
    const float xSizeOfAnchor       = containerSize.x * anchor.min.x;
    const float xPosition           = std::max(xPositionWithOffset + xSizeOfAnchor, containerPosition.x);

    const float yPositionWithOffset = containerPosition.y + anchor.minOffset.y;
    const float ySizeOfAnchor       = containerSize.y * anchor.min.y;
    const float yPosition           = std::max(yPositionWithOffset + ySizeOfAnchor, containerPosition.y);

    return ImVec2(xPosition, yPosition);
}

ImVec2 gluten::element::get_anchor_end_position(const ImVec2& startPosition,
                                                const ImVec2& containerPosition,
                                                const ImVec2& containerSize,
                                                const anchor_info& anchor)
{

    const ImVec2 anchorMax = anchor.max - anchor.min;

    const float xSizeOfAnchor = containerSize.x * anchorMax.x;
    const float xPosition     = std::min(startPosition.x + xSizeOfAnchor + anchor.maxOffset.x, startPosition.x + containerSize.x);

    const float ySizeOfAnchor = containerSize.y * anchorMax.y;
    const float yPosition     = std::min(startPosition.y + ySizeOfAnchor + anchor.maxOffset.y, startPosition.y + containerSize.y);

    return ImVec2(xPosition, yPosition);
}

std::pair<ImRect, ImRect> gluten::element::get_element_start_position(const ImVec2& anchorStartPosition,
                                                                      const ImVec2& anchorEndPosition,
                                                                      const ImVec2& minSize,
                                                                      const ImVec2& desiredSize,
                                                                      const ImVec2& alignment,
                                                                      const ImVec4& padding)
{
    const ImVec2 desiredEnd = anchorStartPosition + desiredSize;
    const ImVec2 minEnd     = anchorStartPosition + minSize;

    // Check each axis independently so a one-axis stretch (e.g. stretch_top
    // or stretch_left) constrains only the stretched axis while the other
    // axis is driven by content/min size.
    const float endX = anchorEndPosition.x > anchorStartPosition.x
        ? anchorEndPosition.x
        : std::max(desiredEnd.x, minEnd.x);

    const float endY = anchorEndPosition.y > anchorStartPosition.y
        ? anchorEndPosition.y
        : std::max(desiredEnd.y, minEnd.y);

    const ImVec2 end(endX, endY);

    const ImVec2 size = end - anchorStartPosition;

    const ImVec2 elementStart = anchorStartPosition - (alignment * size);

    const ImVec2 paddingMin(padding.x, padding.y);
    const ImVec2 paddingMax(padding.z, padding.w);

    return {ImRect{elementStart + paddingMin, elementStart + size - paddingMax}, ImRect(elementStart, elementStart + size)};
}

std::pair<ImRect, ImRect> gluten::element::get_element_box_from_parent(const ImRect& parent,
                                                                       const ImVec2& minSize,
                                                                       const ImVec2& desiredSize,
                                                                       const ImVec2& alignment,
                                                                       const ImVec4& padding,
                                                                       const anchor_info& anchor)
{
    const ImVec2 anchorStart = get_anchor_start_position(parent.Min, parent.GetSize(), anchor);
    const ImVec2 anchorEnd   = get_anchor_end_position(anchorStart, parent.Min, parent.GetSize(), anchor);

    return get_element_start_position(anchorStart, anchorEnd, minSize, desiredSize, alignment, padding);
}