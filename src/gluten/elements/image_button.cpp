#include "image_button.h"

gluten::image_button::image_button(const char* name, const void* data, std::size_t dataSize)
    : m_button(name), m_image(data, dataSize)
{
}

bool gluten::image_button::render()
{
    const ImVec2 cursorBegin   = ImGui::GetCursorScreenPos();
    const bool buttonActivated = m_button.render_invisibile();
    m_image.set_element_parent_info({cursorBegin, m_button.get_element_size()});
    m_image.render();
    return buttonActivated;
}