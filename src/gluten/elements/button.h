#pragma once

#include "gluten/elements/element.h"
#include "gluten/images/image.h"

namespace gluten
{
	class button : public element
	{
    public:
        button() = delete;
        button(const char* name);

        bool render();
        bool render_invisibile();

	private:
        const char* m_name = nullptr;
	};
}