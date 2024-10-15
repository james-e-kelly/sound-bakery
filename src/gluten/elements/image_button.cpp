#include "image_button.h"

gluten::image_button::image_button(const char* name, const void* data, std::size_t dataSize)
    : m_button(name, true), m_image(data, dataSize)
{
}

auto gluten::image_button::get_element_content_size() -> ImVec2 const { return m_image.get_element_content_size(); }

bool gluten::image_button::render_element(const ImRect& parent)
{
    const ImVec2 cursorBegin   = ImGui::GetCursorScreenPos();
    const bool buttonActivated = m_button.render_element(parent);
    m_image.render_element(parent);
    return buttonActivated;
}

void gluten::image_button::set_element_max_size(const ImVec2& maxSize)
{
    element::set_element_max_size(maxSize);
    m_button.set_element_max_size(maxSize);
    m_image.set_element_max_size(maxSize);
}