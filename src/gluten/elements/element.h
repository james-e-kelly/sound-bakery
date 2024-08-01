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
		enum class anchor_preset
		{
            /*
			* ---------
			* | x     |
			* |       |
			* |       |
			* ---------
			*/
			left_top,
            /*
             * ---------
             * |   x   |
             * |       |
             * |       |
             * ---------
             */
			center_top,
            /*
             * ---------
             * |     x |
             * |       |
             * |       |
             * ---------
             */
			right_top,

			/*
             * ---------
             * |       |
             * | x     |
             * |       |
             * ---------
             */
			left_middle,
            /*
             * ---------
             * |       |
             * |   x   |
             * |       |
             * ---------
             */
			center_middle,
            /*
             * ---------
             * |       |
             * |     x |
             * |       |
             * ---------
             */
			right_middle,

			/*
             * ---------
             * |       |
             * |       |
             * | x     |
             * ---------
             */
			left_bottom,
            /*
             * ---------
             * |       |
             * |       |
             * |   x   |
             * ---------
             */
			center_bottom,
            /*
             * ---------
             * |       |
             * |       |
             * |     x |
             * ---------
             */
			right_bottom,

			/*
             * ---------
             * |xxxxxxx|
             * |       |
             * |       |
             * ---------
             */
			stretch_top,
            /*
             * ---------
             * |       |
             * |xxxxxxx|
             * |       |
             * ---------
             */
			stretch_middle,
            /*
             * ---------
             * |       |
             * |       |
             * |xxxxxxx|
             * ---------
             */
			stretch_bottom,

			/*
             * ---------
             * | x     |
             * | x     |
             * | x     |
             * ---------
             */
			stretch_left,
            /*
             * ---------
             * |   x   |
             * |   x   |
             * |   x   |
             * ---------
             */
			stretch_center,
            /*
             * ---------
             * |     x |
             * |     x |
             * |     x |
             * ---------
             */
			stretch_right,

            /*
             * ---------
             * | x x x |
             * | x x x |
             * | x x x |
             * ---------
             */
			stretch_full
		};

		struct anchor_info
		{
            ImVec2 min; //< Start of the element in the range of 0-1
            ImVec2 max; //< End of the element in the range of 0-1
            ImVec2 minOffset;
            ImVec2 maxOffset;

            std::optional<anchor_preset> anchorPreset;  //< Possible preset, if using one
                    
            void set_achor_from_preset(const anchor_preset& preset);
		};

		struct box
		{
            ImVec2 start;
            ImVec2 size;

            ImVec2 end() const;
		};

        struct start_end
        {
            ImVec2 start;
            ImVec2 end;
        };

		void set_element_background_color(ImU32 color);

        anchor_info& get_element_anchor();

        /**
         * @brief If the element has rendered before, return the box
         */
        box get_element_box() const
        {
            return m_currentBox.has_value() ? m_currentBox.value() : box{ImGui::GetWindowPos(), ImVec2()};
        }

		bool render(const box& parent);

        void set_element_alignment(const ImVec2& alignment) { m_alignment = alignment; }

	protected:
        void set_element_desired_size(const ImVec2& desiredSize) { m_desiredSize = desiredSize; }

		virtual bool render_element(const box& parent) = 0;

        static ImVec2 get_anchor_start_position(const ImVec2& containerPosition,
                                            const ImVec2& containerSize,
                                            const anchor_info& anchor);
        static ImVec2 get_anchor_end_position(const ImVec2& startPosition,
                                        const ImVec2& containerPosition,
                                        const ImVec2& containerSize,
                                        const anchor_info& anchor);
        static start_end get_element_start_position(const ImVec2& anchorStartPosition,
                                          const ImVec2& anchorEndPosition,
                                          const ImVec2& minSize,
                                          const ImVec2& desiredSize,
                                          const ImVec2& alignment);
        static box get_element_box_from_parent(const box& parent,
                                           const ImVec2& minSize,
                                           const ImVec2& desiredSize,
                                           const ImVec2& alignment,
                                           const anchor_info& anchor);

        anchor_info m_anchor;
        
        ImVec2 m_minSize;
        ImVec2 m_desiredSize;

        ImVec2 m_selfSize		= ImVec2(32, 32);
        ImVec2 m_offset			= ImVec2(0, 0);
        ImVec2 m_alignment      = ImVec2(0, 0);
        
        std::optional<ImU32> m_backgroundColor;

        std::optional<box> m_currentBox;

        public:
        static inline bool s_debug = false;
	};
}