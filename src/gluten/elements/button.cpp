#include "button.h"

#include "imgui_internal.h"

#include "gluten/theme/theme.h"

gluten::button::button(const char* name) : m_name(name) {}

bool gluten::button::render() 
{ 
    const bool activated = ImGui::Button(m_name, get_element_size()); 
    ImGui::DebugDrawItemRect(gluten::theme::invalidPrefab);
    return activated;
}

bool gluten::button::render_invisibile() 
{ 
	const bool activated = ImGui::InvisibleButton(m_name, get_element_size());
    ImGui::DebugDrawItemRect(gluten::theme::invalidPrefab);
    return activated;
}

gluten::image_button::image_button(const char* name, const void* data, std::size_t dataSize)
    : m_button(name), m_image(data, dataSize)
{
}


bool gluten::image_button::render() 
{ 
	const bool buttonActivated = m_button.render_invisibile(); 
	m_image.render();
    return buttonActivated;
}