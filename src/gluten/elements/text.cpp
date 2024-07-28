#include "text.h"

void gluten::text::render_unformatted(const char* displayText)
{ 
	const ImVec2 textSize = ImGui::CalcTextSize(displayText); 
	set_element_size(textSize);

    ImGui::SetCursorPos(get_element_start());
    ImGui::TextUnformatted(displayText);
}

void gluten::text::render(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    ImGui::TextV(fmt, args);
    va_end(args);
}