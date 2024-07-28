#pragma once

#include "gluten/elements/element.h"

namespace gluten
{
    class text : public element
	{
    public:
        void render_unformatted(const char* displayText);
        void render(const char* fmt, ...);
	};
}