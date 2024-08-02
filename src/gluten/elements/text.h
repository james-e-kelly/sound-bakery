#pragma once

#include "gluten/elements/element.h"

namespace gluten
{
    class text : public element
	{
    public:
        text() = default;
        text(const std::string& displayText);

        void set_text(const std::string& displayText);

        bool render_element(const ImRect& parent) override;

    private:
        std::string m_displayText;
	};
}