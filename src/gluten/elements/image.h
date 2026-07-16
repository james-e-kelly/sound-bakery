#pragma once

#include "gluten/pch.h"
#include "gluten/elements/element.h"

#include <cmrc/cmrc.hpp>

namespace gluten
{
    struct image_destroyer
    {
        void operator()(unsigned char* data);
    };

    enum class image_render
    {
        square,
        circular
    };

    class image : public element
    {
    public:
        using data_ptr = std::unique_ptr<unsigned char, image_destroyer>;

        image() = default;
        image(uint32_t imageText, int width, int height);
        image(const cmrc::embedded_filesystem& filesystem, const std::string& filePath);
        image(const void* data, std::size_t dataSize);
        ~image();

        bool render_element(const element_render_info& renderInfo) override;
        void release();

        auto get_element_content_size(const ImVec2& parentSize) -> ImVec2 const override;
        auto get_width() -> int const { return m_width; }
        auto get_height() -> int const { return m_height; }

        auto set_image_size(const ImVec2& size) -> void
        {
            m_width = size.x;
            m_height = size.y;
        }

        auto set_render_type(image_render render) -> void;

        static auto load_image_data(unsigned char* data, int dataLength, int& width, int& height) -> data_ptr;

    private:
        auto get_image_data(unsigned char* data, int dataLength) -> data_ptr;
        void bind_image_data(const data_ptr& imageData);

        uint32_t m_openGlId = 0;
        int m_width         = 0;
        int m_height        = 0;
        image_render m_render = image_render::square;
        bool m_ownsTexture    = true;
    };
}  // namespace gluten
