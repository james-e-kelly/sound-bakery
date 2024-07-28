#include "element.h"

static ImVec2 operator+(ImVec2 lhs, ImVec2 rhs) { return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y); }

ImVec2 gluten::element::get_element_start() const
{
    ImVec2 start = ImGui::GetItemRectMin();

    switch (m_sizeType)
    {
        case size_type::self:
            start = ImGui::GetItemRectMin() + m_offset;
    }

    return start;
}

ImVec2 gluten::element::get_element_end() const
{
    ImVec2 end = ImGui::GetItemRectMax();

    switch (m_sizeType)
    {
        case size_type::self:
            end = ImGui::GetItemRectMin() + m_selfSize + m_offset;
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