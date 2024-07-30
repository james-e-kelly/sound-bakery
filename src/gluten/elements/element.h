#pragma once

#include "gluten/pch.h"
#include "imgui.h"

namespace gluten
{
	/**
	 * @brief Defines a UI element.
	 * 
	 * Contains offsets, sizing, grouping and so on.
	*/
	class element
	{
    public:
		enum class size_type
		{
			self,	//< element defines its own size
			fill	//< element fills parent item's size
		};

		enum class vertical_alignment
		{
			none,
			top,
			middle,
			bottom
		};

		enum class horizontal_aligntment
		{
			none,
			left,
			center,
			right
		};

		struct parent_info
		{
            ImVec2 parentStart;
            ImVec2 parentSize;

			float parent_center_horizontal() const;
			float parent_right_horizontal() const;
			float parent_center_vertical() const;
			float parent_bottom_vertical() const;
		};

		ImVec2 get_element_start() const;			//< Returns draw start position for the element
        ImVec2 get_element_end() const;				//< Return draw end position for the element
		ImVec2 get_element_size() const;			//< Return the element total size

		void set_element_size(ImVec2 size);			//< Set the element size, if sizing type is set to self
        void set_element_offset(ImVec2 offset);		//< Set the element offset
        void set_element_size_type(size_type type);	//< Set the sizing type of the element
        void set_element_vertical_alignment(vertical_alignment alignment);
        void set_element_horizontal_alignment(horizontal_aligntment alignment);
        void set_element_parent_info(const parent_info& parentInfo);

		virtual bool render_element() = 0;

	private:
        ImVec2 m_selfSize		= ImVec2(32, 32);
        ImVec2 m_offset			= ImVec2(0, 0);
        
		size_type m_sizeType	= size_type::fill;
        vertical_alignment m_verticalAlignment = vertical_alignment::none;
        horizontal_aligntment m_horizontalAlignment = horizontal_aligntment::none;

		std::optional<parent_info> m_parentInfo;
	};
}