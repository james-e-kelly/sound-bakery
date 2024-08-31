#pragma once

#include "pch.h"
#include <cmrc/cmrc.hpp>
#include "stb_image.h"
#include "gluten/elements/element.h"

namespace gluten 
{
	struct image_destroyer
	{
        void operator()(unsigned char* data)
        {
            stbi_image_free(data);
        }
	};

	class image : public element
	{
	public:
		using data_ptr = std::unique_ptr<unsigned char, image_destroyer>;

        image(const cmrc::embedded_filesystem& filesystem, const std::string& filePath);
        image(const void* data, std::size_t dataSize);
        ~image();

		bool render_element(const ImRect& parent) override;
        void release();

		int get_width() const { return m_width; }
		int get_height() const { return m_height; }

		static auto load_image_data(unsigned char* data, int dataLength, int& width, int& height) -> data_ptr;

	private:
        auto get_image_data(unsigned char* data, int dataLength) -> data_ptr;
        void bind_image_data(const data_ptr& imageData);

		uint32_t m_openGlId = 0;
		int m_width = 0;
        int m_height = 0;
	};
}