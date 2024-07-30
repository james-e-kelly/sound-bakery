#include "button.h"

#include "imgui_internal.h"

#include "gluten/theme/theme.h"

gluten::button::button(const char* name, bool invisible, std::function<void()> onActivateFunction) 
    : m_name(name), 
    m_invisible(invisible), 
    m_function(onActivateFunction) {}

bool gluten::button::render_element()
{
    const bool activated =
        m_invisible ? ImGui::InvisibleButton(m_name, get_element_size()) : ImGui::Button(m_name, get_element_size());

    if (activated && m_function)
    {
        m_function();
    }
    return activated;
}