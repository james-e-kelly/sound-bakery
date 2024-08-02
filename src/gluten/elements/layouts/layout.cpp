#include "layout.h"

static ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y); }
static ImVec2 operator/(const ImVec2& lhs, const float& rhs) { return ImVec2(lhs.x / rhs, lhs.y / rhs); }
static ImVec2 operator*(const ImVec2& lhs, const float& rhs) { return ImVec2(lhs.x * rhs, lhs.y * rhs); }

gluten::layout::layout(const layout_type& layoutType) : m_layoutType(layoutType) 
{

}

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

    if (!m_currentLayoutPos.has_value())
    {
        m_currentLayoutPos = thisBox.Min;
    }

    const ImVec2 sizeGivenToElement = ImVec2(horizontalPixels, verticalPixels);

    if (element)
    {
        activated = element->render({m_currentLayoutPos.value(), m_currentLayoutPos.value() + sizeGivenToElement});
    }

    m_currentLayoutPos = m_currentLayoutPos.value() + sizeGivenToElement;

    return activated;
}

void gluten::layout::reset_layout(const ImRect& parent)
{
    const ImRect elementBox = get_element_box_from_parent(parent, m_minSize, m_desiredSize, m_alignment, m_anchor);
    m_currentRect           = elementBox;
    m_currentLayoutPos      = elementBox.Min;
}