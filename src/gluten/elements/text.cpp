#include "text.h"

gluten::text::text(const std::string& displayText) : m_displayText(displayText) 
{
    set_element_desired_size(ImGui::CalcTextSize(displayText.c_str()));
}

gluten::text::text(const std::string& displayText, const ImVec2& alignment, const anchor_preset& anchorPreset)
    : element(anchorPreset), 
    m_displayText(displayText)
{
    set_element_desired_size(ImGui::CalcTextSize(displayText.c_str()));
    m_alignment = alignment;
}

void gluten::text::set_text(const std::string& displayText) 
{ 
    m_displayText = displayText; 
    
    set_element_desired_size(ImGui::CalcTextSize(displayText.c_str()));
}

bool gluten::text::render_element(const ImRect& parent)
{
    if (!m_displayText.empty())
    {
        ImGui::TextUnformatted(m_displayText.c_str());
    }

    return false;
}