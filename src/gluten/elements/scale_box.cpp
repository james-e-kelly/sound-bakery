#include "scale_box.h"

gluten::scale_box::scale_box(const anchor_preset& anchorPreset) : element(anchorPreset) {}

auto gluten::scale_box::set_aspect_ratio(float ratio) -> scale_box&
{
    m_aspectRatio = ratio;
    return *this;
}

bool gluten::scale_box::render_child(element* child)
{
    if (m_lastBox.has_value())
    {
        const ImRect childRect = compute_child_rect(m_lastBox.value());
        return child->render(childRect);
    }

    m_pendingChild = child;
    return render_window();
}

auto gluten::scale_box::render_element(const element_render_info& renderInfo) -> bool
{
    m_lastBox = renderInfo.elementBox;

    if (!m_pendingChild)
    {
        return false;
    }

    const ImRect childRect = compute_child_rect(renderInfo.elementBox);
    const bool result      = m_pendingChild->render(childRect);
    m_pendingChild         = nullptr;
    return result;
}

ImRect gluten::scale_box::compute_child_rect(const ImRect& box) const
{
    const ImVec2 size = box.GetSize();

    if (size.x <= 0.0f || size.y <= 0.0f || m_aspectRatio <= 0.0f)
    {
        return box;
    }

    const bool stretchesX = m_anchor.max.x > m_anchor.min.x;
    const bool stretchesY = m_anchor.max.y > m_anchor.min.y;

    float childW = size.x;
    float childH = size.y;

    if (stretchesY && !stretchesX)
    {
        childH = size.y;
        childW = size.y * m_aspectRatio;
    }
    else if (stretchesX && !stretchesY)
    {
        childW = size.x;
        childH = size.x / m_aspectRatio;
    }
    else
    {
        const float fitByWidth  = size.x / m_aspectRatio;
        const float fitByHeight = size.y * m_aspectRatio;

        if (fitByWidth <= size.y)
        {
            childW = size.x;
            childH = fitByWidth;
        }
        else
        {
            childW = fitByHeight;
            childH = size.y;
        }
    }

    const float anchorX = stretchesX ? 0.5f : m_anchor.min.x;
    const float anchorY = stretchesY ? 0.5f : m_anchor.min.y;

    const float offsetX = (size.x - childW) * anchorX;
    const float offsetY = (size.y - childH) * anchorY;

    return ImRect(box.Min.x + offsetX, box.Min.y + offsetY, box.Min.x + offsetX + childW, box.Min.y + offsetY + childH);
}
