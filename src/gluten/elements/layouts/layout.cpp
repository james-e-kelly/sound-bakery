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
    const element::box elementBox = get_element_box();

    return render_layout_element_internal(elementBox, element, horizontalPixels, verticalPixels);
}

bool gluten::layout::render_layout_element_pixels_horizontal(element* element, float horizontalPixels)
{
    const element::box elementBox = get_element_box();

    return render_layout_element_internal(elementBox, element, horizontalPixels, elementBox.size.y);
}

bool gluten::layout::render_layout_element_pixels_vertical(element* element, float verticalPixels)
{
    const element::box elementBox = get_element_box();

    return render_layout_element_internal(elementBox, element, elementBox.size.x, verticalPixels);
}

bool gluten::layout::render_layout_element_percent(element* element,
                                           float horizontalPercent,
                                           float verticalPercent)
{
    const element::box elementBox = get_element_box();

    return render_layout_element_internal(elementBox, element, elementBox.size.x * horizontalPercent,
                                        elementBox.size.y * verticalPercent);
}

bool gluten::layout::render_layout_element_percent_horizontal(element* element, float horizontalPercent)
{
    const element::box elementBox = get_element_box();

    return render_layout_element_internal(elementBox, element, elementBox.size.x * horizontalPercent, elementBox.size.y);
}

bool gluten::layout::render_layout_element_percent_vertical(element* element, float verticalPercent)
{
    const element::box elementBox = get_element_box();

    return render_layout_element_internal(elementBox, element, elementBox.size.x, elementBox.size.y * verticalPercent);
}

bool gluten::layout::render_layout_element_internal(const element::box& thisBox,
                                                    element* element,
                                                    float horizontalPixels,
                                                    float verticalPixels)
{
    bool activated = false;

    if (!m_currentLayoutPos.has_value())
    {
        m_currentLayoutPos = thisBox.start;
    }

    const ImVec2 sizeGivenToElement = ImVec2(horizontalPixels, verticalPixels);

    if (element)
    {
        activated = element->render({m_currentLayoutPos.value(), sizeGivenToElement});
    }

    m_currentLayoutPos = m_currentLayoutPos.value() + sizeGivenToElement;

    return activated;
}