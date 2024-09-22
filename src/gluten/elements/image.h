#pragma once

#include "pch.h"
#include <cmrc/cmrc.hpp>
#include "gluten/elements/element.h"

namespace gluten 
{
	struct image_destroyer
	{
        void operator()(unsigned char* data);
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

		auto get_element_content_size() -> ImVec2 const override { return ImVec2(get_width(), get_height()); }
		auto get_width() -> int const { return m_width; }
		auto get_height() -> int const { return m_height; }

		static auto load_image_data(unsigned char* data, int dataLength, int& width, int& height) -> data_ptr;

	private:
        auto get_image_data(unsigned char* data, int dataLength) -> data_ptr;
        void bind_image_data(const data_ptr& imageData);

		uint32_t m_openGlId = 0;
		int m_width = 0;
        int m_height = 0;
	};
}