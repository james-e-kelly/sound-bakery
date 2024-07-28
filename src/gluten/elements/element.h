#pragma once

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

		ImVec2 get_element_start() const;			//< Returns draw start position for the element
        ImVec2 get_element_end() const;				//< Return draw end position for the element
		ImVec2 get_element_size() const;			//< Return the element total size

		void set_element_size(ImVec2 size);			//< Set the element size, if sizing type is set to self
        void set_element_offset(ImVec2 offset);		//< Set the element offset
        void set_element_size_type(size_type type);	//< Set the sizing type of the element

	private:
        ImVec2 m_selfSize		= ImVec2(16, 16);
        ImVec2 m_offset			= ImVec2(0, 0);
        size_type m_sizeType	= size_type::fill;
	};
}