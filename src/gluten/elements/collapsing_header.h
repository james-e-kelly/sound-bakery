#pragma once

#include "gluten/pch.h"
#include "gluten/elements/element.h"

namespace gluten
{
	class collapsing_header : public gluten::element
	{
    public:
        collapsing_header(const std::string& label, bool defaultOpen = true) 
		: element(anchor_preset::stretch_full), m_label(label), m_defaultOpen(defaultOpen)
		{
		}

	protected:
        auto render_element(const ImRect& parentRect) -> bool override;
		auto get_element_content_size(const ImVec2& parentSize) -> ImVec2 const override
		{
            const ImVec2 textSize = ImGui::CalcTextSize(m_label.c_str());
			return ImVec2(0.0f, textSize.y + (ImGui::GetStyle().FramePadding.y * 2.0f));
		}

		std::string m_label;
        bool m_defaultOpen = true;
	};
}