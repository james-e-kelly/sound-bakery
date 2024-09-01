#include "button.h"

#include "imgui_internal.h"

#include "gluten/theme/theme.h"

gluten::button::button(const char* name, bool invisible, std::function<void()> onActivateFunction) 
    : m_name(name), 
    m_invisible(invisible), 
    m_function(onActivateFunction) {}

bool gluten::button::render_element(const ImRect& elementRect)
{
    const ImVec2 size = elementRect.GetSize();

    if (size.x < 0.1f || size.y < 0.1f)
    {
        return false;
    }

    if (m_invisible)
    {
        ImGui::SetNextItemAllowOverlap();
    }

    const bool activated =
        m_invisible ? ImGui::InvisibleButton(m_name, elementRect.GetSize()) : ImGui::Button(m_name, elementRect.GetSize());

    if (activated && m_function)
    {
        m_function();
    }
    return activated;
}