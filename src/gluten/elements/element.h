#pragma once

#include "gluten/pch.h"
#include "imgui.h"
#include "imgui_internal.h"

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

        element() = default;
        element(const anchor_preset& anchorPreset);
        ~element();

        void set_font_size(float size);
        void set_element_scale(float scale);

		void set_element_background_color(ImU32 color);
        void set_element_hover_color(ImU32 color);

        anchor_info& get_element_anchor();
        ImVec2 get_element_desired_size() const;

        /**
         * @brief If the element has rendered before, return the box
         */
        ImRect get_element_rect() const;
        ImRect get_element_rect_local() const; //< If the element has rendered before, return the box local to the current window

		bool render(const ImRect& parent);
        bool render_window();   //< Render using the window as the container

        void set_element_alignment(const ImVec2& alignment) { m_alignment = alignment; }
        virtual void set_element_max_size(const ImVec2& maxSize);

	protected:
        virtual auto get_element_content_size() -> ImVec2 const { return ImVec2(0, 0); }

        virtual auto pre_render_element() -> void {}
		virtual auto render_element(const ImRect& elementBox) -> bool { return false; }

        static ImVec2 get_anchor_start_position(const ImVec2& containerPosition,
                                            const ImVec2& containerSize,
                                            const anchor_info& anchor);
        static ImVec2 get_anchor_end_position(const ImVec2& startPosition,
                                        const ImVec2& containerPosition,
                                        const ImVec2& containerSize,
                                        const anchor_info& anchor);
        static ImRect get_element_start_position(const ImVec2& anchorStartPosition,
                                          const ImVec2& anchorEndPosition,
                                          const ImVec2& minSize,
                                          const ImVec2& desiredSize,
                                          const ImVec2& alignment);
        static ImRect get_element_box_from_parent(const ImRect& parent,
                                           const ImVec2& minSize,
                                           const ImVec2& desiredSize,
                                           const ImVec2& alignment,
                                           const anchor_info& anchor);

        anchor_info m_anchor;
        
        ImVec2 m_minSize;
        ImVec2 m_maxSize = ImVec2(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
        ImVec2 m_alignment;     //< defines the pivot point of the element in each axis from 0-1
        
        std::optional<ImU32> m_backgroundColor;
        std::optional<ImU32> m_hoverColor;
        std::optional<ImRect> m_currentRect;
        
        float m_scale = 1.0f;

    public:
        static inline bool s_debug = false;
	};
}