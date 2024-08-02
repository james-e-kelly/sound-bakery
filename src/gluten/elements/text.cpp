#include "text.h"

gluten::text::text(const std::string& displayText) : m_displayText(displayText) {}

void gluten::text::set_text(const std::string& displayText) 
{ 
    m_displayText = displayText; 
    
    set_element_desired_size(ImGui::CalcTextSize(displayText.c_str()));
}

bool gluten::text::render_element(const ImRect& parent)
{
    if (!m_displayText.empty())
    {
        const ImVec2 textSize = ImGui::CalcTextSize(m_displayText.c_str());

        ImGui::SetCursorPos(parent.Min);
        ImGui::TextUnformatted(m_displayText.c_str());
    }

    return false;
}