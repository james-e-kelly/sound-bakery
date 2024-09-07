#pragma once

#include "gluten/elements/element.h"

namespace gluten
{
	class menu_bar : public element
	{
    public:
        menu_bar();
        ~menu_bar();

        virtual bool render_element(const ImRect& elementBox) override;
        void end_menu_bar();

	private:
        bool m_hasEnded = false;
	};
}