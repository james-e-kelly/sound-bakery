#pragma once

#include "gluten/elements/button.h"
#include "gluten/images/image.h"

namespace gluten
{
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