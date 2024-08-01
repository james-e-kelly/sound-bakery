#include "layout.h"

static ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y); }
static ImVec2 operator/(const ImVec2& lhs, const float& rhs) { return ImVec2(lhs.x / rhs, lhs.y / rhs); }
static ImVec2 operator*(const ImVec2& lhs, const float& rhs) { return ImVec2(lhs.x * rhs, lhs.y * rhs); }

gluten::layout::layout(const layout_type& layoutType) : m_layoutType(layoutType) 
{

}

void gluten::layout::set_layout_type(const layout_type& type) { m_layoutType = type; }

bool gluten::layout::render_layout_element(element* element,
                                           float horizontalPercent,
                                           float verticalPercent)
{
    bool activated = false;

    const element::box elementBox = get_element_box();

    // If first render, set the layout start
    if (!m_currentLayoutPos.has_value())
    {
        m_currentLayoutPos = elementBox.start;
    }

    const ImVec2 sizeGivenToElement =
        ImVec2(elementBox.size.x * horizontalPercent, elementBox.size.y * verticalPercent);

    if (element)
    {
        activated = element->render({m_currentLayoutPos.value(), sizeGivenToElement});
    }

    m_currentLayoutPos = m_currentLayoutPos.value() + sizeGivenToElement;

    return activated;
}