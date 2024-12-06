#include "element.h"

#include "gluten/theme/carbon_colors.h"

static ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y); }
static ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y); }
static ImVec2 operator-(const ImVec2& lhs) { return ImVec2(-lhs.x, -lhs.y); }

static ImVec2 operator+=(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y); }

static ImVec2 operator*(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x * rhs.x, lhs.y * rhs.y); }
static ImVec2 operator*(const ImVec2& lhs, float rhs) { return ImVec2(lhs.x * rhs, lhs.y * rhs); }

static bool operator>(const ImVec2& lhs, const ImVec2& rhs) { return lhs.x > rhs.x || lhs.y > rhs.y; }

static ImVec2 max_from_vec(const ImVec2& lhs, const ImVec2& rhs)
{
    return ImVec2(lhs.x > rhs.x ? lhs.x : rhs.x, lhs.y > rhs.y ? lhs.y : rhs.y);
}

static ImVec2 clamp_vec(const ImVec2& max, const ImVec2& input)
{
    return ImVec2(input.x > max.x ? max.x : input.x, input.y > max.y ? max.y : input.y);
}

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

gluten::element::element(const anchor_preset& anchorPreset) { m_anchor.set_achor_from_preset(anchorPreset); }

gluten::element::~element()
{
    if (ImGui::GetCurrentContext())
    {
        ImGui::SetWindowFontScale(1.0f);
    }
}

void gluten::element::set_font_size(float size)
{
    const float currentFontSize = ImGui::GetFontSize();

    const float scale = size / currentFontSize;

    set_element_scale(scale);
}

void gluten::element::set_element_scale(float scale) { m_scale = scale; }

void gluten::element::set_element_background_color(ImU32 color) { m_backgroundColor = color; }

void gluten::element::set_element_hover_color(ImU32 color) { m_hoverColor = color; }

void gluten::element::set_element_padding(const ImVec2& padding) { m_padding = padding; }

void gluten::element::set_element_window_padding() { m_padding = ImGui::GetStyle().WindowPadding; }

void gluten::element::set_element_frame_padding() { m_padding = ImGui::GetStyle().FramePadding; }

gluten::element::anchor_info& gluten::element::get_element_anchor() { return m_anchor; }

ImRect gluten::element::get_element_rect() const
{
    if (!m_currentRect.has_value())
    {
        m_currentRect = ImGui::GetCurrentWindow()->WorkRect;
    }
    return m_currentRect.value();
}

ImRect gluten::element::get_element_rect_local() const
{
    ImRect elementRect = get_element_rect();
    elementRect.Translate(-ImGui::GetWindowPos());
    return elementRect;
}

bool gluten::element::render(const ImRect& parent)
{
    pre_render_element();

    if (m_scale.has_value())
    {
        ImGui::SetWindowFontScale(m_scale.value());
    }

    const ImRect elementBox =
        get_element_box_from_parent(parent, m_minSize, get_element_content_size(), m_alignment, m_padding, m_anchor);
    m_currentRect = elementBox;

    if (s_debug)
    {
        ImDrawList* const foregroundDrawList = ImGui::GetForegroundDrawList();
        foregroundDrawList->AddRect(elementBox.Min, elementBox.Max,
                                    ImGui::ColorConvertFloat4ToU32(gluten::theme::red50));
    }

    ImGui::SetCursorScreenPos(elementBox.Min);

    const bool activated = render_element(elementBox);

    const bool hovered = ImGui::IsItemHovered();

    if (ImDrawList* const backgroundDrawList = ImGui::GetBackgroundDrawList())
    {
        if (hovered && m_hoverColor.has_value())
        {
            backgroundDrawList->AddRectFilled(elementBox.Min, elementBox.Max, m_hoverColor.value());
        }
        else if (m_backgroundColor.has_value())
        {
            backgroundDrawList->AddRectFilled(elementBox.Min, elementBox.Max, m_backgroundColor.value());

            ImVec2 bottomLeft  = elementBox.GetBL();
            ImVec2 bottomRight = elementBox.GetBR();
            bottomLeft.x       = (int)bottomLeft.x;
            bottomLeft.y       = (int)bottomLeft.y;
            bottomRight.x      = (int)bottomRight.x;
            bottomRight.y      = (int)bottomRight.y;

            const ImGuiStyle& style = ImGui::GetStyle();
            ImGui::GetCurrentWindow()->DrawList->AddLine(bottomLeft, bottomRight,
                                                         ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Border]),
                                                         style.WindowBorderSize);
        }
    }

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

void gluten::element::set_element_max_size(const ImVec2& maxSize) { m_maxSize = maxSize; }

ImVec2 gluten::element::get_anchor_start_position(const ImVec2& containerPosition,
                                                  const ImVec2& containerSize,
                                                  const anchor_info& anchor)
{
    const float xPositionWithOffset = containerPosition.x + anchor.minOffset.x;
    const float xSizeOfAnchor       = containerSize.x * anchor.min.x;
    const float xPosition           = xPositionWithOffset + xSizeOfAnchor;

    const float yPositionWithOffset = containerPosition.y + anchor.minOffset.y;
    const float ySizeOfAnchor       = containerSize.y * anchor.min.y;
    const float yPosition           = yPositionWithOffset + ySizeOfAnchor;

    return ImVec2(xPosition, yPosition);
}

ImVec2 gluten::element::get_anchor_end_position(const ImVec2& startPosition,
                                                const ImVec2& containerPosition,
                                                const ImVec2& containerSize,
                                                const anchor_info& anchor)
{

    const ImVec2 anchorMax = anchor.max - anchor.min;

    const float xSizeOfAnchor = containerSize.x * anchorMax.x;
    const float xPosition     = startPosition.x + xSizeOfAnchor + anchor.maxOffset.x;

    const float ySizeOfAnchor = containerSize.y * anchorMax.y;
    const float yPosition     = startPosition.y + ySizeOfAnchor + anchor.maxOffset.y;

    return ImVec2(xPosition, yPosition);
}

ImRect gluten::element::get_element_start_position(const ImVec2& anchorStartPosition,
                                                   const ImVec2& anchorEndPosition,
                                                   const ImVec2& minSize,
                                                   const ImVec2& desiredSize,
                                                   const ImVec2& alignment,
                                                   const ImVec2& padding)
{
    const ImVec2 desiredEnd = anchorStartPosition + desiredSize;
    const ImVec2 minEnd     = anchorStartPosition + minSize;

    ImVec2 end;

    // If anchor end is set, the max is always the anchor end
    if (anchorEndPosition > anchorStartPosition)
    {
        end = clamp_vec(anchorEndPosition, max_from_vec(anchorEndPosition, max_from_vec(desiredEnd, minEnd)));
    }
    // If anchor end is NOT set, the desired size and min size take over
    else
    {
        end = max_from_vec(desiredEnd, minEnd);
    }

    const ImVec2 size = end - anchorStartPosition;

    const ImVec2 elementStart = anchorStartPosition - (alignment * size);

    return ImRect{elementStart + padding, elementStart + size - padding};
}

ImRect gluten::element::get_element_box_from_parent(const ImRect& parent,
                                                    const ImVec2& minSize,
                                                    const ImVec2& desiredSize,
                                                    const ImVec2& alignment,
                                                    const ImVec2& padding,
                                                    const anchor_info& anchor)
{
    const ImVec2 anchorStart = get_anchor_start_position(parent.Min, parent.GetSize(), anchor);
    const ImVec2 anchorEnd   = get_anchor_end_position(anchorStart, parent.Min, parent.GetSize(), anchor);

    return get_element_start_position(anchorStart, anchorEnd, minSize, desiredSize, alignment, padding);
}