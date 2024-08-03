#include "layout.h"

static ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y); }
static ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y); }
static ImVec2 operator/(const ImVec2& lhs, const float& rhs) { return ImVec2(lhs.x / rhs, lhs.y / rhs); }
static ImVec2 operator*(const ImVec2& lhs, const float& rhs) { return ImVec2(lhs.x * rhs, lhs.y * rhs); }

gluten::layout::layout(const layout_type& layoutType) : m_layoutType(layoutType) {}

void gluten::layout::set_layout_type(const layout_type& type) { m_layoutType = type; }

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

bool gluten::layout::render_layout_element_percent(element* element,
                                           float horizontalPercent,
                                           float verticalPercent)
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

    bool firstLayoutRender = m_firstLayout;

    if (!m_currentLayoutPos.has_value())
    {
        setup_layout_begin(thisBox);
    }

    ImVec2 currentLayoutPos = m_currentLayoutPos.value();

    if (element)
    {
        if (firstLayoutRender)
        {
            if (m_layoutType == layout_type::right_to_left)
            {
                currentLayoutPos.x -= sizeGivenToElement.x;
            }
            else if (m_layoutType == layout_type::bottom_to_top)
            {
                currentLayoutPos.y -= sizeGivenToElement.y;
            }
        }

        activated = element->render({currentLayoutPos, currentLayoutPos + sizeGivenToElement});
    }

    switch (m_layoutType)
    {
        case gluten::layout::layout_type::left_to_right:
            currentLayoutPos.x += sizeGivenToElement.x;
            break;
        case gluten::layout::layout_type::right_to_left:
            currentLayoutPos.x -= sizeGivenToElement.x;
            break;
        case gluten::layout::layout_type::top_to_bottom:
            currentLayoutPos.y += sizeGivenToElement.y;
            break;
        case gluten::layout::layout_type::bottom_to_top:
            currentLayoutPos.y -= sizeGivenToElement.y;
            break;
        default:
            break;
    }
    
    m_currentLayoutPos = currentLayoutPos;

    m_firstLayout = false;

    return activated;
}

void gluten::layout::reset_layout(const ImRect& parent)
{
    const ImRect elementBox = get_element_box_from_parent(parent, m_minSize, m_desiredSize, m_alignment, m_anchor);
    m_currentRect           = elementBox;
    setup_layout_begin(elementBox);
    m_firstLayout = true;
}

void gluten::layout::setup_layout_begin(const ImRect& thisBox)
{
    if (m_layoutType == layout_type::left_to_right || m_layoutType == layout_type::top_to_bottom)
    {
        m_currentLayoutPos = thisBox.Min;
    }
    else if (m_layoutType == layout_type::right_to_left)
    {
        m_currentLayoutPos = ImVec2(thisBox.Max.x, thisBox.Min.y);
    }
    else if (m_layoutType == layout_type::bottom_to_top)
    {
        m_currentLayoutPos = ImVec2(thisBox.Min.x, thisBox.Max.y);
    }
}