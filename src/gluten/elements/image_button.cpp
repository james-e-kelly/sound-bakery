#include "image_button.h"

gluten::image_button::image_button(const char* name, const void* data, std::size_t dataSize)
    : m_button(name, true), m_image(data, dataSize)
{
    set_element_desired_size(m_image.get_element_desired_size());
}

bool gluten::image_button::render_element(const ImRect& parent)
{
    const ImVec2 cursorBegin   = ImGui::GetCursorScreenPos();
    const bool buttonActivated = m_button.render_element(parent);
    m_image.render_element(parent);
    return buttonActivated;
}