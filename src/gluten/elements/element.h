#pragma once

#include "gluten/pch.h"

#include "gluten/theme/theme.h"

namespace gluten
{
    enum class border_sides : int
    {
        none   = 0,
        top    = 1 << 0,
        bottom = 1 << 1,
        left   = 1 << 2,
        right  = 1 << 3,
        all    = top | bottom | left | right
    };

    constexpr border_sides operator|(border_sides a, border_sides b) { return static_cast<border_sides>(static_cast<int>(a) | static_cast<int>(b)); }
    constexpr border_sides operator&(border_sides a, border_sides b) { return static_cast<border_sides>(static_cast<int>(a) & static_cast<int>(b)); }
    constexpr border_sides operator~(border_sides a) { return static_cast<border_sides>(~static_cast<int>(a)); }
    constexpr bool operator!(border_sides a) { return static_cast<int>(a) == 0; }

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

    /**
     * @brief Defines percentage and pixel information.
     */
    struct anchor_info
    {
        ImVec2 min;        //< Start of the element in the range of 0-1 percentage
        ImVec2 max;        //< End of the element in the range of 0-1 percentage
        ImVec2 minOffset;  //< Pixel offset of the min/element start
        ImVec2 maxOffset;  //< Pixel offset of the max/element end

        auto set_min_offset(ImVec2 offset) -> anchor_info&;
        auto set_max_offset(ImVec2 offset) -> anchor_info&;

        std::optional<anchor_preset> anchorPreset;  //< Possible preset, if using one

        void set_achor_from_preset(const anchor_preset& preset);
    };

    /**
     * @brief Information passed to an element when rendering.
     * 
     * Mainly passes the ImRect box to render inside, plus additional variables.
     * 
     * Used to keep the @ref element::render_element function stable.
     */
    struct element_render_info
    {
        ImRect elementBox;  //< Box the element should render inside
        bool isVisible;     //< Whether this element is visible. If invisible, you can skip rendering and just add dummy elements to keep the same sizing
    };

    /**
     * @brief Defines a UI element.
     *
     * Contains offsets, sizing, grouping and so on.
     * 
     * Elements are inspired from Unreal Engine's widgets.
     * Each widget contains a @ref anchor_info which lets elements render themselves in relation to a parent rect.
     * 
     * Elements are not replacements for standard ImGui calls like ImGui::TextUnformatted.
     * Standard calls can be used like normal.
     * Elements can be used to render text like "render text in the top right corner with some padding and 5px away from the right".
     */
    class element
    {
        LEAK_DETECTOR(element)
            
    public:
        using anchor_preset = ::gluten::anchor_preset;
        using anchor_info   = ::gluten::anchor_info;

        element();
        element(const anchor_preset& anchorPreset);
        virtual ~element();

        auto has_element_scale() const -> bool;
        auto get_element_scale() const -> float;

        auto virtual set_element_content_font_size(float size) -> element&;                     //< Uses the passed in size and current font size to scale the content scale
        auto virtual set_element_content_scale(float scale) -> element&;                        //< Set the content scale. Content scale is used by text, buttons, sliders, etc.
        auto virtual set_element_scale(float scale) -> element&;                                //< Set an element scale that scales the rendering rect.
        auto virtual set_element_background_color(ImU32 color) -> element&;
        auto virtual set_element_background_color(ImVec4 color) -> element&;
        auto virtual set_element_hover_color(ImU32 color) -> element&;
        auto virtual set_element_hover_color(ImVec4 color) -> element&;
        auto virtual set_element_active_color(ImU32 color) -> element&;
        auto virtual set_element_active_color(ImVec4 color) -> element&;
        auto virtual set_element_active(bool active) -> element&;
        auto virtual set_element_padding(const ImVec2& padding) -> element&;
        auto virtual set_element_padding(const ImVec4& padding) -> element&;
        auto virtual set_element_window_padding() -> element&;
        auto virtual set_element_frame_padding() -> element&;
        auto virtual set_element_inner_padding(const ImVec2& padding) -> element&;
        auto virtual set_element_inner_padding(const ImVec4& padding) -> element&;
        auto virtual set_element_inner_padding(float padding) -> element&;
        auto virtual set_element_alignment(const ImVec2& alignment) -> element&;
        auto virtual set_element_anchor_preset(const anchor_preset& preset) -> element&;
        auto virtual set_element_min_size(const ImVec2& minSize) -> element&;
        auto virtual set_element_max_size(const ImVec2& maxSize) -> element&;
        auto virtual set_element_translation(const ImVec2& translation) -> element&;
        auto virtual set_element_border(float borderSize, float borderRounding) -> element&;
        auto virtual set_element_border_sides(border_sides sides) -> element&;
        auto virtual set_element_border_color(ImU32 color) -> element&;
        auto virtual set_element_border_color(ImVec4 color) -> element&;
        auto virtual set_element_rounding(float rounding) -> element&;
        auto virtual set_element_rounding_flags(ImDrawFlags flags) -> element&;

        /**
         * @brief Give this element a stable id used for animation state.
         *
         * Animation state (see gluten::animation) is looked up by ImGuiID.
         * If not set, the element's animated features are silently disabled
         * because there is no way to persist state for an element that is
         * typically re-constructed every frame.
         */
        auto virtual set_animation_id(std::string_view id) -> element&;

        /**
         * @brief Enable a hover-driven content-scale animation.
         *
         * When the pointer is over this element, its content scale smoothly
         * eases toward @p scaleWhenHovered, then back to 1.0 when it leaves.
         * Applies on top of any base content scale you have already set.
         *
         * Requires set_animation_id() so per-instance state can persist.
         *
         * @param scaleWhenHovered Target multiplier. 1.15 = grow by 15%.
         * @param rate             Smoothing rate; higher = snappier.
         */
        auto virtual set_hover_grow(float scaleWhenHovered, float rate = 15.0f) -> element&;

        anchor_info& get_element_anchor();

        ImRect get_element_rect() const;                //< If the element has rendered before, return the box
        ImRect get_element_content_rect() const;        //< Element rect inset by inner padding
        ImRect get_element_rect_local() const;          //< If the element has rendered before, return the box local to the current window

        auto get_element_is_hovered() const -> bool;    //< Returns true if the element's box is hovered by the mouse. @see get_element_rect

        bool render(const ImRect& parent);              //< Render the element into the parent rect
        bool render_window();                           //< Render using the window as the container
        bool render_cursor();                           //< Render where the draw cursor is currently

        virtual auto get_element_content_size(const ImVec2& parentSize = ImVec2(0, 0)) -> ImVec2 const { return ImVec2(0, 0); }

        /**
         * @brief Called on construction and any time an owning widget wants to refresh its style.
         */
        auto virtual refresh_element() -> void {}

    protected:
        virtual auto pre_render_element() -> void {}
        virtual auto post_render_element() -> void {}
        virtual auto render_element(const element_render_info& renderInfo) -> bool { return false; }

        static ImVec2 get_anchor_start_position(const ImVec2& containerPosition, const ImVec2& containerSize, const anchor_info& anchor);
        static ImVec2 get_anchor_end_position(const ImVec2& startPosition, const ImVec2& containerPosition, const ImVec2& containerSize, const anchor_info& anchor);
        static std::pair<ImRect, ImRect> get_element_start_position(const ImVec2& anchorStartPosition, const ImVec2& anchorEndPosition, const ImVec2& minSize, const ImVec2& desiredSize, const ImVec2& alignment, const ImVec4& padding);
        static std::pair<ImRect, ImRect> get_element_box_from_parent(const ImRect& parent, const ImVec2& minSize, const ImVec2& desiredSize, const ImVec2& alignment, const ImVec4& padding, const anchor_info& anchor);

        anchor_info m_anchor;

        ImVec2 m_translation;
        ImVec2 m_minSize;
        ImVec2 m_maxSize = ImVec2(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
        ImVec2 m_alignment;  //< defines the pivot point of the element in each axis from 0-1
        ImVec4 m_padding;
        ImVec4 m_innerPadding;

        std::optional<ImU32> m_backgroundColor;
        std::optional<ImU32> m_hoverColor;
        std::optional<ImU32> m_activeColor;
        std::optional<float> m_borderSize;
        std::optional<float> m_borderRounding;
        border_sides m_borderSides = border_sides::all;
        std::optional<ImU32> m_borderColor;
        float m_elementRounding = 0.0f;
        ImDrawFlags m_drawFlags = ImDrawFlags_RoundCornersAll;
        mutable std::optional<ImRect> m_currentRect;

        bool m_active = false;

        std::optional<float> m_contentScale;
        float m_scale = 1.0f;

        std::string m_animationId;
        std::optional<float> m_hoverGrowScale;
        float m_hoverGrowRate = 15.0f;

    public:
        static inline bool s_debug           = false;
        static inline bool s_debugVertical   = false;
        static inline bool s_debugHorizontal = false;
    };

    class background : public element
    {
    public:
        background() : element(anchor_preset::stretch_full) {}
        background(const anchor_preset& anchorPreset) : element(anchorPreset) {}

    protected:
        auto render_element(const element_render_info& renderInfo) -> bool override { return false; }
    };
}  // namespace gluten