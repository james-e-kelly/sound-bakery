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