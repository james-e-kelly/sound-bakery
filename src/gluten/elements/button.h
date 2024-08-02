#pragma once

#include "gluten/elements/element.h"

namespace gluten
{
	class button : public element
	{
    public:
        button() = delete;
        button(const char* name, bool invisible = false, std::function<void()> onActivateFunction = std::function<void()>());

        bool render_element(const ImRect& parent) override;

	private:
        const char* m_name = nullptr;
        const bool m_invisible = false;
        std::function<void()> m_function;
	};
}