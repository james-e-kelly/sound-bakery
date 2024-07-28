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

	class image_button
	{
    public:
        image_button() = delete;
        image_button(const char* name, const void* data, std::size_t dataSize);

        bool render();

        button& get_button() { return m_button; }
        image& get_image() { return m_image; }

    private:
		button m_button;
        image m_image;
	};
}