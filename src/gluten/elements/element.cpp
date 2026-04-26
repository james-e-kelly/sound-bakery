#include "element.h"

#include "gluten/theme/carbon/carbon_colors.h"

static ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y); }
static ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y); }
static ImVec2 operator-(const ImVec2& lhs) { return ImVec2(-lhs.x, -lhs.y); }

static ImVec2 operator+=(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y); }

static ImVec2 operator*(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x * rhs.x, lhs.y * rhs.y); }
static ImVec2 operator*(const ImVec2& lhs, float rhs) { return ImVec2(lhs.x * rhs, lhs.y * rhs); }

static ImRect operator+(const ImRect& lhs, const ImVec2& rhs) { return ImRect(lhs.Min + rhs, lhs.Max + rhs); }
static ImRect operator*(const ImRect& lhs, const float& rhs) 
{ 
    ImVec2 expandSize = (lhs.GetSize() * rhs) - lhs.GetSize();
    ImRect result(lhs);
    result.Expand(expandSize);
    return result;
}

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

gluten::element::element(const anchor_preset& anchorPreset) { m_anchor.set_achor_from_preset(anchorPreset); }

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
    m_padding = padding; 
    return *this;
}

auto gluten::element::set_element_window_padding() -> element& 
{ 
    m_padding = ImGui::GetStyle().WindowPadding; 
    return *this;
}

auto gluten::element::set_element_frame_padding() -> element& 
{ 
    m_padding = ImGui::GetStyle().FramePadding; 
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

ImRect gluten::element::get_element_rect_local() const
{
    ImRect elementRect = get_element_rect();
    elementRect.Translate(-ImGui::GetWindowPos());
    return elementRect;
}

bool gluten::element::render(const ImRect& parent)
{
    pre_render_element();

    if (m_contentScale.has_value())
    {
        ImGui::SetWindowFontScale(m_contentScale.value());
    }
    else
    {
        ImGui::SetWindowFontScale(1.0f);
    }

    const std::pair<ImRect, ImRect> elementBoxes = get_element_box_from_parent(parent, m_minSize, get_element_content_size(parent.GetSize()), m_alignment, m_padding, m_anchor);
    const ImRect elementBox = (elementBoxes.first * m_scale) + m_translation;
    const ImRect elementBoxNoPadding = (elementBoxes.second * m_scale) + m_translation;
    m_currentRect = elementBox;
    
    ImDrawList* const windowDrawList     = ImGui::GetWindowDrawList();

    ImGui::SetCursorScreenPos(elementBoxNoPadding.Min);
    ImGui::Dummy(elementBoxNoPadding.GetSize()); 
    ImGui::SetCursorScreenPos(elementBox.Min);

    const bool isItemVisible = ImGui::IsRectVisible(elementBox.Min, elementBox.Max);
    const bool hovered = ImGui::IsMouseHoveringRect(elementBox.Min, elementBox.Max);

    if (windowDrawList && m_borderSize.has_value())
    {
        const float borderSize = m_borderSize.value();

        if (borderSize > 0.0f)
        {
            const ImU32 borderColor    = ImGui::GetColorU32(ImGuiCol_Border);
            const float borderRounding = ImGui::GetStyle().FrameRounding;

            ImRect borderRect = elementBox;
            borderRect.Expand(borderSize);

            windowDrawList->AddRectFilled(borderRect.GetTL(), borderRect.GetBR(), borderColor, borderRounding);
        }
    }

    if (m_active && !hovered && m_activeColor.has_value())
    {
        if (windowDrawList)
        {
            windowDrawList->AddRectFilled(elementBox.Min, elementBox.Max, m_activeColor.value(), m_elementRounding);
        }
    }
    else if (hovered && m_hoverColor.has_value())
    {
        if (windowDrawList)
        {
            windowDrawList->AddRectFilled(elementBox.Min, elementBox.Max, m_hoverColor.value(), m_elementRounding);
        }
    }
    else if (m_backgroundColor.has_value())
    {
        if (ImDrawList* const backgroundDrawList = ImGui::GetBackgroundDrawList())
        {
            windowDrawList->AddRectFilled(elementBox.Min, elementBox.Max, m_backgroundColor.value(), m_elementRounding);
        }
    }

    element_render_info renderInfo;
    renderInfo.elementBox = elementBox;
    renderInfo.isVisible  = isItemVisible;

    const bool activated = render_element(renderInfo);

    if (activated && hovered && m_activeColor.has_value())
    {
        if (windowDrawList)
        {
            windowDrawList->AddRectFilled(elementBox.Min, elementBox.Max, m_activeColor.value(), m_elementRounding);
        }
    }

    if (s_debug)
    {
        windowDrawList->AddRect(elementBox.Min, elementBox.Max, ImGui::ColorConvertFloat4ToU32(gluten::theme::red50), m_elementRounding);
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

auto gluten::element::set_element_rounding(float rounding) -> element&
{
    m_elementRounding = rounding;
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

    return {ImRect{elementStart + padding, elementStart + size - padding}, ImRect(elementStart, elementStart + size)};
}

std::pair<ImRect, ImRect> gluten::element::get_element_box_from_parent(const ImRect& parent,
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