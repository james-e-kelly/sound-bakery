#include "element.h"

#include "gluten/theme/theme.h"

static ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y); }
static ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y); }

static ImVec2 operator+=(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y); }

static ImVec2 operator*(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x * rhs.x, lhs.y * rhs.y); }

static ImVec2 max_from_vec(const ImVec2& lhs, const ImVec2& rhs)
{
    return ImVec2(lhs.x > rhs.x ? lhs.x : rhs.x, lhs.y > rhs.y ? lhs.y : rhs.y);
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

void gluten::element::set_element_background_color(ImU32 color)
{ 
    m_backgroundColor = color; 
}

gluten::element::anchor_info& gluten::element::get_element_anchor() { return m_anchor; }

bool gluten::element::render(const ImRect& parent)
{ 
    const ImRect elementBox = get_element_box_from_parent(parent, m_minSize, m_desiredSize, m_alignment, m_anchor);
    m_currentRect         = elementBox;

    if (s_debug)
    {
        ImDrawList* const foregroundDrawList = ImGui::GetForegroundDrawList();
        foregroundDrawList->AddRect(elementBox.Min, elementBox.Max, gluten::theme::invalidPrefab);
    }

    ImGui::SetCursorScreenPos(elementBox.Min);

    const bool activated = render_element(elementBox); 

    if (m_backgroundColor.has_value())
    {
        ImGui::SetCursorScreenPos(elementBox.Min);

        ImDrawList* const backgroundDrawList = ImGui::GetBackgroundDrawList();
        backgroundDrawList->AddRectFilled(elementBox.Min, elementBox.Max, m_backgroundColor.value());
    }

    return activated;
}

ImVec2 gluten::element::get_anchor_start_position(const ImVec2& containerPosition,
                                 const ImVec2& containerSize,
                                 const anchor_info& anchor)
{
    const float xPositionWithOffset = containerPosition.x + anchor.minOffset.x;
    const float xSizeOfAnchor      = containerSize.x * anchor.min.x;
    const float xPosition           = xPositionWithOffset + xSizeOfAnchor;

    const float yPositionWithOffset = containerPosition.y + anchor.minOffset.y;
    const float ySizeOfAnchor      = containerSize.y * anchor.min.y;
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
                                                                       const ImVec2& alignment)
{
    const ImVec2 desiredEnd = anchorStartPosition + desiredSize;
    const ImVec2 minEnd     = anchorStartPosition + minSize;

    const ImVec2 end = max_from_vec(anchorEndPosition, max_from_vec(desiredEnd, minEnd));

    const ImVec2 size = end - anchorStartPosition;

    const ImVec2 elementStart = anchorStartPosition - (alignment * size);

    return ImRect{elementStart, elementStart + size};
}

ImRect gluten::element::get_element_box_from_parent(const ImRect& parent,
                                                    const ImVec2& minSize,
                                                    const ImVec2& desiredSize,
                                                    const ImVec2& alignment,
                                                    const anchor_info& anchor)
{
    const ImVec2 anchorStart = get_anchor_start_position(parent.Min, parent.GetSize(), anchor);
    const ImVec2 anchorEnd   = get_anchor_end_position(anchorStart, parent.Min, parent.GetSize(), anchor);

    return get_element_start_position(anchorStart, anchorEnd, minSize, desiredSize, alignment);
}