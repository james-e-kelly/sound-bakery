#include "element.h"

static ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y); }

static ImVec2 operator+=(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y); }

ImVec2 gluten::element::get_element_start() const
{
    ImVec2 start = ImGui::GetItemRectMin();

    if (m_parentInfo.has_value() && m_sizeType == size_type::self)
    {
        switch (m_horizontalAlignment)
        {
            case horizontal_aligntment::none:
                break;
            case horizontal_aligntment::left:
                start.x = m_parentInfo.value().parentStart.x;
                break;
            case horizontal_aligntment::center:
                start.x = m_parentInfo.value().parent_center_horizontal() - (get_element_size().x / 2.0f);
                break;
            case horizontal_aligntment::right:
                start.x = m_parentInfo.value().parent_right_horizontal() - get_element_size().x;
        }

        switch (m_verticalAlignment)
        {
            case vertical_alignment::none:
                break;
            case vertical_alignment::top:
                start.y = m_parentInfo.value().parentStart.y;
                break;
            case vertical_alignment::middle:
                start.y = m_parentInfo.value().parent_center_vertical() - (get_element_size().y / 2.0f);
                break;
            case vertical_alignment::bottom:
                start.y = m_parentInfo.value().parent_bottom_vertical() - (get_element_size().y);
                break;
        }
    }

    switch (m_sizeType)
    {
        case size_type::self:
            start += m_offset;
            break;
    }

    return start;
}

ImVec2 gluten::element::get_element_end() const
{
    ImVec2 end = ImGui::GetItemRectMax();

    switch (m_sizeType)
    {
        case size_type::self:
            end = get_element_start() + get_element_size() + m_offset;
            break;
        case size_type::fill:
            end = ImGui::GetItemRectMax();
            break;
    }

    return end;
}

ImVec2 gluten::element::get_element_size() const
{
    switch (m_sizeType)
    {
        case size_type::self:
            return m_selfSize;
        case size_type::fill:
            return ImGui::GetItemRectSize();
    }

    return ImGui::GetItemRectSize();
}

void gluten::element::set_element_size(ImVec2 size)
{ m_selfSize = size; }

void gluten::element::set_element_offset(ImVec2 offset)
{ m_offset = offset; }

void gluten::element::set_element_size_type(gluten::element::size_type type)
{ m_sizeType = type; }

void gluten::element::set_element_vertical_alignment(vertical_alignment alignment)
{ m_verticalAlignment = alignment; }

void gluten::element::set_element_horizontal_alignment(horizontal_aligntment alignment)
{ m_horizontalAlignment = alignment; }

void gluten::element::set_element_parent_info(const parent_info& parentInfo) { m_parentInfo = parentInfo; }

float gluten::element::parent_info::parent_center_horizontal() const
{
    return parentStart.x + (parentSize.x / 2.0f);
}

float gluten::element::parent_info::parent_right_horizontal() const 
{ 
    return parentStart.x + parentSize.x; 
}

float gluten::element::parent_info::parent_center_vertical() const
{
    return parentStart.y + (parentSize.y / 2.0f);
}

float gluten::element::parent_info::parent_bottom_vertical() const 
{ 
    return parentStart.y + parentSize.y; 
}
