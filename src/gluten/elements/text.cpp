#include "text.h"

gluten::text::text(const std::string& displayText) : m_displayText(displayText) {}

void gluten::text::set_text(const std::string& displayText) 
{ 
    m_displayText = displayText; 
}

bool gluten::text::render_element() 
{
    if (!m_displayText.empty())
    {
        const ImVec2 textSize = ImGui::CalcTextSize(m_displayText.c_str());
        set_element_size(textSize);

        ImGui::SetCursorPos(get_element_start());
        ImGui::TextUnformatted(m_displayText.c_str());
    }

    return false;
}