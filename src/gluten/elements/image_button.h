#pragma once

#include "gluten/elements/button.h"
#include "gluten/elements/image.h"

namespace gluten
{
    class image_button : public element
    {
    public:
        image_button() = delete;
        image_button(const char* name, const void* data, std::size_t dataSize);

        bool render_element() override;

        button& get_button() { return m_button; }
        image& get_image() { return m_image; }

    private:
        button m_button;
        image m_image;
    };
}