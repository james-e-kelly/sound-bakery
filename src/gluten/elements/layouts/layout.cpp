#include "layout.h"

#include "gluten/theme/carbon_theme_g100.h"

static ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y); }
static ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y); }
static ImVec2 operator/(const ImVec2& lhs, const float& rhs) { return ImVec2(lhs.x / rhs, lhs.y / rhs); }
static ImVec2 operator*(const ImVec2& lhs, const float& rhs) { return ImVec2(lhs.x * rhs, lhs.y * rhs); }

gluten::layout::layout(const layout_type& layoutType) : m_layoutType(layoutType) {}

gluten::layout::layout(const layout_type& layoutType, const anchor_preset& anchorPreset)
    : element(anchorPreset), m_layoutType(layoutType)
{
}

gluten::layout::layout(const anchor_preset& anchorPreset)
    : element(anchorPreset), m_layoutType(layout_type::left_to_right)
{
}

void gluten::layout::set_layout_type(const layout_type& type) { m_layoutType = type; }

void gluten::layout::set_layout_spacing(float spacing) { m_spacing = spacing; }

void gluten::layout::render_spacer_pixels(float horizonalPixels, float verticalPixels)
{
    render_layout_element_pixels(nullptr, horizonalPixels, verticalPixels);
}

void gluten::layout::render_spacer_percent(float horizontalPercent, float verticalPercent)
{
    render_layout_element_percent(nullptr, horizontalPercent, verticalPercent);
}

bool gluten::layout::render_layout_element_full(element* element)
{
    const ImRect elementBox = get_element_rect();

    return render_layout_element_internal(elementBox, element, elementBox.GetSize().x, elementBox.GetSize().y);
}

bool gluten::layout::render_layout_element_pixels(element* element, float horizontalPixels, float verticalPixels)
{
    const ImRect elementBox = get_element_rect();

    return render_layout_element_internal(elementBox, element, horizontalPixels, verticalPixels);
}

bool gluten::layout::render_layout_element_pixels_horizontal(element* element, float horizontalPixels)
{
    const ImRect elementBox = get_element_rect();

    return render_layout_element_internal(elementBox, element, horizontalPixels, elementBox.GetSize().y);
}

bool gluten::layout::render_layout_element_pixels_vertical(element* element, float verticalPixels)
{
    const ImRect elementBox = get_element_rect();

    return render_layout_element_internal(elementBox, element, elementBox.GetSize().x, verticalPixels);
}

bool gluten::layout::render_layout_element_percent(element* element, float horizontalPercent, float verticalPercent)
{
    const ImRect elementBox = get_element_rect();

    return render_layout_element_internal(elementBox, element, elementBox.GetSize().x * horizontalPercent,
                                          elementBox.GetSize().y * verticalPercent);
}

bool gluten::layout::render_layout_element_percent_horizontal(element* element, float horizontalPercent)
{
    const ImRect elementBox = get_element_rect();

    return render_layout_element_internal(elementBox, element, elementBox.GetSize().x * horizontalPercent,
                                          elementBox.GetSize().y);
}

bool gluten::layout::render_layout_element_percent_vertical(element* element, float verticalPercent)
{
    const ImRect elementBox = get_element_rect();

    return render_layout_element_internal(elementBox, element, elementBox.GetSize().x,
                                          elementBox.GetSize().y * verticalPercent);
}

bool gluten::layout::render_layout_element_internal(const ImRect& thisBox,
                                                    element* element,
                                                    float horizontalPixels,
                                                    float verticalPixels)
{
    bool activated = false;

    const ImVec2 sizeGivenToElement = ImVec2(horizontalPixels, verticalPixels);

    const bool firstLayoutRender = m_firstLayout;
    m_firstLayout                = false;

    if (!m_currentLayoutPos.has_value())
    {
        setup_layout_begin(thisBox);
    }

    ImVec2 currentLayoutPos = m_currentLayoutPos.value();

    if (firstLayoutRender)
    {
        // "backwards" layouts still render left to right
        // so start one element over
        if (m_layoutType == layout_type::right_to_left)
        {
            currentLayoutPos.x -= sizeGivenToElement.x;
        }
        else if (m_layoutType == layout_type::bottom_to_top)
        {
            currentLayoutPos.y -= sizeGivenToElement.y;
        }
    }

    // If rendering would go outside the element box
    // drop a new row or column
    const ImVec2 thisSize = thisBox.GetSize();
    switch (m_layoutType)
    {
        case layout::layout_type::left_to_right:
            if (thisSize.x > 1.0f)  // if we have no size, allow going outside our box
            {
                if (currentLayoutPos.x + sizeGivenToElement.x > thisBox.GetTR().x)
                {
                    currentLayoutPos.x = thisBox.Min.x;
                    currentLayoutPos.y += sizeGivenToElement.y + m_spacing;
                }
            }
            break;
    }

    if (element)
    {
        if (s_debug)
        {
            ImDrawList* const foregroundDrawList = ImGui::GetForegroundDrawList();
            foregroundDrawList->AddRect(currentLayoutPos, currentLayoutPos + sizeGivenToElement,
                                        ImGui::ColorConvertFloat4ToU32(gluten::theme::purple50));
        }

        activated = element->render({currentLayoutPos, currentLayoutPos + sizeGivenToElement});
    }

    switch (m_layoutType)
    {
        case gluten::layout::layout_type::left_to_right:
            currentLayoutPos.x += sizeGivenToElement.x + m_spacing;
            break;
        case gluten::layout::layout_type::right_to_left:
            currentLayoutPos.x -= sizeGivenToElement.x + m_spacing;
            break;
        case gluten::layout::layout_type::top_to_bottom:
            currentLayoutPos.y += sizeGivenToElement.y + m_spacing;
            break;
        case gluten::layout::layout_type::bottom_to_top:
            currentLayoutPos.y -= sizeGivenToElement.y + m_spacing;
            break;
        default:
            break;
    }

    m_currentLayoutPos = currentLayoutPos;

    return activated;
}

void gluten::layout::reset_layout(const ImRect& parent)
{
    const ImRect elementBox =
        get_element_box_from_parent(parent, m_minSize, get_element_content_size(), m_alignment, m_padding, m_anchor);
    m_currentRect = elementBox;
    setup_layout_begin(elementBox);
    m_firstLayout = true;
}

void gluten::layout::finish_layout()
{
    if (m_currentRect.has_value())
    {
        ImGui::SetCursorScreenPos(m_currentRect.value().GetBL());
    }
}

ImVec2 gluten::layout::get_current_layout_pos_local() const
{
    ImVec2 layoutPos = get_current_layout_pos();
    return layoutPos - ImGui::GetWindowPos();
}

void gluten::layout::setup_layout_begin(const ImRect& thisBox)
{
    if (m_layoutType == layout_type::left_to_right || m_layoutType == layout_type::top_to_bottom)
    {
        m_currentLayoutPos = thisBox.GetTL();
    }
    else if (m_layoutType == layout_type::right_to_left)
    {
        m_currentLayoutPos = thisBox.GetTR();
    }
    else if (m_layoutType == layout_type::bottom_to_top)
    {
        m_currentLayoutPos = thisBox.GetBL();
    }
}