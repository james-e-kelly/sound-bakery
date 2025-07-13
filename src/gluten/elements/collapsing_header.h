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

		std::string m_label;
        bool m_defaultOpen = true;
	};
}