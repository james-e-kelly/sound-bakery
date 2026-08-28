#pragma once

#include "gluten/pch.h"

namespace gluten
{
    enum class fonts
    {
        regular,
        regular_font_awesome,
        regular_audio_icons,
        regular_lucide_icons,
        light,
        semibold,
        title,
        title_lucide_icons
    };

    /**
     * @brief Text styles that map to a font, font size, and line height.
     *
     * @see theme.h
     */
    enum class text_style
    {
        caption,        //< small meta text, timestamps, keycaps, field helpers
        helper,         //< field labels, table headers (semibold, tracked)
        label,
        body,           //< Default text
        body_strong,    //< Default text with a stronger font
        subtitle,       //< Supporting paragraph directly under a header. Sized like h4 but regular weight
        h4,             //< panel / group titles (CollapsingHeader)
        h3,             //< tab-content section headings
        h2,             //< dialog / modal titles
        h1              //< settings pages, welcome screens (rare)
    };

    /**
     * @brief Button styles that map to colors for the fill, hover, active, and text.
     */
    enum class button_style
    {
        primary,        //< Filled with the interactive color (blue). Main CTA.
        secondary,      //< Filled with the layer color. Lower emphasis.
        ghost,          //< Transparent background, visible only on hover.
        danger          //< Filled with the error color. Destructive actions.
    };

    namespace theme
    {
        constexpr ImU32 invalidPrefab = IM_COL32(222, 43, 43, 255);

        template <typename T>
        concept arithmetic = std::is_arithmetic_v<T>;

        template <arithmetic T>
        constexpr auto abs(T x) noexcept -> decltype(std::abs(x))
        {
            if (std::is_constant_evaluated())
            {
                return x < 0 ? -x : x;
            }
            else
            {
                return std::abs(x);
            }
        }

        constexpr auto trunc(float x) noexcept -> int
        {
            return static_cast<int>(x);
        }

        constexpr auto fmod(float x, float y) noexcept -> float
        {
            return x - trunc(x / y) * y;
        }

        // Constexpr version of ImGui::ColorConvertRGBtoHSV
        constexpr auto color_convert_rgb_to_hsv(float r, float g, float b, float& out_h, float& out_s, float& out_v) noexcept -> void
        {
            float K = 0.f;
            if (g < b)
            {
                std::swap(g, b);
                K = -1.f;
            }
            if (r < g)
            {
                std::swap(r, g);
                K = -2.f / 6.f - K;
            }

            const float chroma = r - (g < b ? g : b);
            out_h              = abs(K + (g - b) / (6.f * chroma + 1e-20f));
            out_s              = chroma / (r + 1e-20f);
            out_v              = r;
        }

        constexpr auto color_convert_hsv_to_rbg(float h, float s, float v, float& out_r, float& out_g, float& out_b) noexcept -> void
        {
            if (s == 0.0f)
            {
                // gray
                out_r = out_g = out_b = v;
                return;
            }
            
            h       = fmod(h, 1.0f) / (60.0f / 360.0f);
            int i   = (int)h;
            float f = h - (float)i;
            float p = v * (1.0f - s);
            float q = v * (1.0f - s * f);
            float t = v * (1.0f - s * (1.0f - f));

            switch (i)
            {
                case 0:
                    out_r = v;
                    out_g = t;
                    out_b = p;
                    break;
                case 1:
                    out_r = q;
                    out_g = v;
                    out_b = p;
                    break;
                case 2:
                    out_r = p;
                    out_g = v;
                    out_b = t;
                    break;
                case 3:
                    out_r = p;
                    out_g = q;
                    out_b = v;
                    break;
                case 4:
                    out_r = t;
                    out_g = p;
                    out_b = v;
                    break;
                case 5:
                default:
                    out_r = v;
                    out_g = p;
                    out_b = q;
                    break;
            }
        }

        constexpr auto make_hsv(float h, float s, float v, float a = 1.0f) noexcept -> ImColor 
        {
            float r, g, b;
            color_convert_hsv_to_rbg(h, s, v, r, g, b);
            return ImColor(r, g, b, a);
        }

        constexpr auto color_with_value(const ImColor& color, float value) noexcept -> ImVec4
        {
            const ImVec4& colRow = color.Value;
            float hue, sat, val;
            color_convert_rgb_to_hsv(colRow.x, colRow.y, colRow.z, hue, sat, val);
            return make_hsv(hue, sat, std::min(value, 1.0f)).Value;
        }

        constexpr auto color_with_saturation(const ImColor& color, float saturation) noexcept -> ImVec4 
        {
            const ImVec4& colRow = color.Value;
            float hue, sat, val;
            color_convert_rgb_to_hsv(colRow.x, colRow.y, colRow.z, hue, sat, val);
            return make_hsv(hue, std::min(saturation, 1.0f), val).Value;
        }

        constexpr auto color_with_hue(const ImColor& color, float hue) noexcept -> ImVec4 
        {
            const ImVec4& colRow = color.Value;
            float h, s, v;
            color_convert_rgb_to_hsv(colRow.x, colRow.y, colRow.z, h, s, v);
            return make_hsv(std::min(hue, 1.0f), s, v).Value;
        }

        constexpr auto color_with_multiplied_value(const ImColor& color, float multiplier) noexcept -> ImVec4 
        {
            const ImVec4& colRow = color.Value;
            float hue, sat, val;
            color_convert_rgb_to_hsv(colRow.x, colRow.y, colRow.z, hue, sat, val);
            return make_hsv(hue, sat, std::min(val * multiplier, 1.0f)).Value;
        }

        constexpr auto color_with_multiplied_saturation(const ImColor& color, float multiplier) noexcept -> ImVec4 
        {
            const ImVec4& colRow = color.Value;
            float hue, sat, val;
            color_convert_rgb_to_hsv(colRow.x, colRow.y, colRow.z, hue, sat, val);
            return make_hsv(hue, std::min(sat * multiplier, 1.0f), val).Value;
        }

        constexpr auto color_with_multiplied_hue(const ImColor& color, float multiplier) noexcept -> ImVec4
        {
            const ImVec4& colRow = color.Value;
            float hue, sat, val;
            color_convert_rgb_to_hsv(colRow.x, colRow.y, colRow.z, hue, sat, val);
            return make_hsv(std::min(hue * multiplier, 1.0f), sat, val).Value;
        }

        constexpr auto color_with_added_value(const ImColor& color, float delta) noexcept -> ImVec4 
        {
            const ImVec4& colRow = color.Value;
            float hue, sat, val;
            color_convert_rgb_to_hsv(colRow.x, colRow.y, colRow.z, hue, sat, val);
            return make_hsv(hue, sat, std::clamp(val + delta, 0.0f, 1.0f)).Value;
        }

        constexpr auto color_with_added_saturation(const ImColor& color, float delta) noexcept -> ImVec4 
        {
            const ImVec4& colRow = color.Value;
            float hue, sat, val;
            color_convert_rgb_to_hsv(colRow.x, colRow.y, colRow.z, hue, sat, val);
            return make_hsv(hue, std::min(sat + delta, 1.0f), val).Value;
        }

        constexpr auto color_with_added_hue(const ImColor& color, float delta) noexcept -> ImVec4
        {
            const ImVec4& colRow = color.Value;
            float hue, sat, val;
            color_convert_rgb_to_hsv(colRow.x, colRow.y, colRow.z, hue, sat, val);
            return make_hsv(std::min(hue + delta, 1.0f), sat, val).Value;
        }

        constexpr auto adjust_alpha(const ImVec4& color, const float& alpha) noexcept -> ImVec4 
        {
            return ImVec4(color.x, color.y, color.z, alpha);
        }

        constexpr auto hex_to_imgui_imvec4(const unsigned long& hex) noexcept -> ImVec4 
        {
            constexpr float s = 1.0f / 255.0f;

            // The RGB order is backwards here -> BGR
            // Also, set alpha to 1 as the carbon hex values don't hold alpha and therefore
            // everything would get set to 0
            return ImVec4(((hex >> IM_COL32_B_SHIFT) & 0xFF) * s, ((hex >> IM_COL32_G_SHIFT) & 0xFF) * s,
                          ((hex >> IM_COL32_R_SHIFT) & 0xFF) * s, 1.0f);
        }

        extern ImVec4 white;
        extern ImVec4 black;

        // Background
        extern ImVec4 backgroundLow;
        extern ImVec4 background;
        extern ImVec4 backgroundInverse;
        extern ImVec4 backgroundActive;
        extern ImVec4 backgroundHover;
        extern ImVec4 backgroundInverseHover;
        extern ImVec4 backgroundSelected;
        extern ImVec4 backgroundSelectedHover;

        // Layer
        extern ImVec4 layer01;
        extern ImVec4 layerActive01;
        extern ImVec4 layerHover01;
        extern ImVec4 layerSelected01;
        extern ImVec4 layerSelectedHover01;

        // layer-02 - Use for sidebar/panels (medium depth)
        extern ImVec4 layer02;
        extern ImVec4 layerActive02;
        extern ImVec4 layerHover02;
        extern ImVec4 layerSelected02;
        extern ImVec4 layerSelectedHover02;

        // layer-03 - Use for elevated elements (cards, modals)
        extern ImVec4 layer03;
        extern ImVec4 layerActive03;
        extern ImVec4 layerHover03;
        extern ImVec4 layerSelected03;
        extern ImVec4 layerSelectedHover03;

        // layer
        extern ImVec4 layerSelectedInverse;
        extern ImVec4 layerSelectedDisabled;

        // layer-accent-01
        extern ImVec4 layerAccent01;
        extern ImVec4 layerAccentActive01;
        extern ImVec4 layerAccentHover01;

        // layer-accent-02
        extern ImVec4 layerAccent02;
        extern ImVec4 layerAccentActive02;
        extern ImVec4 layerAccentHover02;

        // layer-accent-03
        extern ImVec4 layerAccent03;
        extern ImVec4 layerAccentActive03;
        extern ImVec4 layerAccentHover03;

        // Field
        // field-01
        extern ImVec4 field01;
        extern ImVec4 fieldHover01;

        // field-02
        extern ImVec4 field02;
        extern ImVec4 fieldHover02;

        // field-03
        extern ImVec4 field03;
        extern ImVec4 fieldHover03;

        // Border
        // border-subtle-00
        extern ImVec4 borderSubtle00;

        // border-subtle-01
        extern ImVec4 borderSubtle01;
        extern ImVec4 borderSubtleSelected01;

        // border-subtle-02
        extern ImVec4 borderSubtle02;
        extern ImVec4 borderSubtleSelected02;

        // border-subtle-03
        extern ImVec4 borderSubtle03;
        extern ImVec4 borderSubtleSelected03;

        // border-strong
        extern ImVec4 borderStrong01;
        extern ImVec4 borderStrong02;
        extern ImVec4 borderStrong03;

        // border-tile
        extern ImVec4 borderTile01;
        extern ImVec4 borderTile02;
        extern ImVec4 borderTile03;

        // border-inverse
        extern ImVec4 borderInverse;

        // border-interactive
        extern ImVec4 borderInteractive;

        // border
        extern ImVec4 borderDisabled;

        // Text
        extern ImVec4 textPrimary;
        extern ImVec4 textSecondary;
        extern ImVec4 textPlaceholder;
        extern ImVec4 textHelper;
        extern ImVec4 textError;
        extern ImVec4 textInverse;
        extern ImVec4 textOnColor;
        extern ImVec4 textOnColorDisabled;
        extern ImVec4 textDisabled;

        // Link
        extern ImVec4 linkPrimary;
        extern ImVec4 linkPrimaryHover;
        extern ImVec4 linkSecondary;
        extern ImVec4 linkInverse;
        extern ImVec4 linkVisited;
        extern ImVec4 linkInverseActive;
        extern ImVec4 linkInverseHover;

        // Icon
        extern ImVec4 iconPrimary;
        extern ImVec4 iconSecondary;
        extern ImVec4 iconInverse;
        extern ImVec4 iconOnColor;
        extern ImVec4 iconOnColorDisabled;
        extern ImVec4 iconDisabled;
        extern ImVec4 iconInteractive;

        // Support
        extern ImVec4 supportError;
        extern ImVec4 supportSuccess;
        extern ImVec4 supportWarning;
        extern ImVec4 supportInfo;

        // Focus
        extern ImVec4 focus;
        extern ImVec4 focusInset;
        extern ImVec4 focusInverse;

        // Misc
        extern ImVec4 interactive;
        extern ImVec4 interactiveHover;
        extern ImVec4 interactiveActive;
        extern ImVec4 interactiveSecondary;
        extern ImVec4 interactiveSecondaryHover;
        extern ImVec4 highlight;
        extern ImVec4 overlay;
        extern ImVec4 toggleOff;
        extern ImVec4 shadow;

        extern ImVec4 missingColorColor;

        constexpr float space04             = 4.0f;
        constexpr float space08             = 8.0f;
        constexpr float space12             = 12.0f;
        constexpr float space16             = 16.0f;
        constexpr float space20             = 20.0f;
        constexpr float space24             = 24.0f;
        constexpr float space32             = 32.0f;
        constexpr float space48             = 48.0f;

        constexpr float radiusSm            = 4.0f;    //< dots, chips, tick marks
        constexpr float radiusMd            = 6.0f;    //< buttons, fields, rows
        constexpr float radiusLg            = 10.0f;   //< panels, popups, plot container
        constexpr float radiusPill          = 999.0f;  //< status pill

        constexpr float textSizeCaption     = 11.0f;
        constexpr float textSizeHelper      = 12.0f;
        constexpr float textSizeLabel       = 12.0f;
        constexpr float textSizeBody        = 14.0f;
        constexpr float textSizeBodyStrong  = 14.0f;
        constexpr float textSizeSubtitle    = 16.0f;
        constexpr float textSizeH4          = 16.0f;
        constexpr float textSizeH3          = 20.0f;
        constexpr float textSizeH2          = 24.0f;
        constexpr float textSizeH1          = 32.0f;

        constexpr float textLineCaption     = 16.0f;
        constexpr float textLineHelper      = 16.0f;
        constexpr float textLineLabel       = 16.0f;
        constexpr float textLineBody        = 20.0f;
        constexpr float textLineBodyStrong  = 20.0f;
        constexpr float textLineSubtitle    = 24.0f;
        constexpr float textLineH4          = 24.0f;
        constexpr float textLineH3          = 28.0f;
        constexpr float textLineH2          = 32.0f;
        constexpr float textLineH1          = 40.0f;

        constexpr float layoutBarHeightBody         = textLineBody + space16 * 2.0f;                    //< top toolbars, breadcrumb bars (body text)            
        constexpr float layoutMetaSlotHeight        = textLineHelper + space04 * 2.0f;                  //< helper-size meta chip rows (vote icons etc.)           
        constexpr float layoutIconButtonSize        = space32;                                         

        extern float layoutTitleBarHeight;                      //< OS-window title bar height. Window recommends 32-48px
        extern float layoutIconRailSize;                        //< Left toolbar icon rails
        extern float layoutIconRailRatio;                       //< Ratio between the titlebar and icon rail size

        constexpr float iconSizeSm                  = 14.0f;    //< inline with helper / caption text
        constexpr float iconSizeMd                  = 18.0f;    //< inline with body text (default)
        constexpr float iconSizeLg                  = 22.0f;    //< inline with subtitle / h4 headings
        constexpr float iconSizeXl                  = 28.0f;    //< large icons for icon-only nav rail buttons

        constexpr ImVec2 insetNone                  = ImVec2(0.0f, 0.0f);
        constexpr ImVec2 insetCompact               = ImVec2(space08, space04);
        constexpr ImVec2 insetFrame                 = ImVec2(space12, space08);
        constexpr ImVec2 insetWindow                = ImVec2(space24, space20);

        constexpr auto text_size_for(text_style style) noexcept -> float
        {
            switch (style)
            {
                case text_style::caption:
                    return textSizeCaption;
                case text_style::helper:
                    return textSizeHelper;
                case text_style::label:
                    return textSizeLabel;
                case text_style::body:
                    return textSizeBody;
                case text_style::body_strong:
                    return textSizeBodyStrong;
                case text_style::subtitle:
                    return textSizeSubtitle;
                case text_style::h4:
                    return textSizeH4;
                case text_style::h3:
                    return textSizeH3;
                case text_style::h2:
                    return textSizeH2;
                case text_style::h1:
                    return textSizeH1;
            }
            return textSizeBody;
        }

        constexpr auto text_line_height_for(text_style style) noexcept -> float
        {
            switch (style)
            {
                case text_style::caption:
                    return textLineCaption;
                case text_style::helper:
                    return textLineHelper;
                case text_style::label:
                    return textLineLabel;
                case text_style::body:
                    return textLineBody;
                case text_style::body_strong:
                    return textLineBodyStrong;
                case text_style::subtitle:
                    return textLineSubtitle;
                case text_style::h4:
                    return textLineH4;
                case text_style::h3:
                    return textLineH3;
                case text_style::h2:
                    return textLineH2;
                case text_style::h1:
                    return textLineH1;
            }
            return textLineBody;
        }

        constexpr auto text_font_for(text_style style) noexcept -> fonts
        {
            switch (style)
            {
                case text_style::h1:
                case text_style::h2:
                case text_style::h3:
                case text_style::h4:
                case text_style::body_strong:
                case text_style::label:
                    return fonts::semibold;
                case text_style::caption:
                case text_style::helper:
                case text_style::body:
                case text_style::subtitle:
                default:
                    return fonts::regular;
            }
            return fonts::regular;
        }

        inline auto text_color_for(text_style style) -> const ImVec4&
        {
            switch (style)
            {
                case text_style::caption:
                case text_style::helper:
                case text_style::subtitle:
                    return textSecondary;
                case text_style::label:
                    return textSecondary;
                case text_style::h1:
                case text_style::h2:
                case text_style::h3:
                case text_style::h4:
                case text_style::body:
                case text_style::body_strong:
                default:
                    return textPrimary;
            }
        }

        struct button_style_colors
        {
            ImVec4 button;
            ImVec4 hovered;
            ImVec4 active;
            ImVec4 text;
        };

        inline button_style_colors button_colors_for(button_style style)
        {
            switch (style)
            {
                case button_style::primary:
                    return {interactive, interactiveHover, interactiveActive, textOnColor};
                case button_style::secondary:
                    return {layer02, layerHover02, layerActive02, textPrimary};
                case button_style::ghost:
                    return {ImVec4(0, 0, 0, 0), backgroundHover, backgroundActive, textPrimary};
                case button_style::danger:
                    return {supportError,
                            color_with_added_value(supportError, 0.08f),
                            color_with_added_value(supportError, 0.12f),
                            textOnColor};
            }
            return {interactive, interactiveHover, interactiveActive, textOnColor};
        }

        inline const ImVec4& icon_color_for(button_style style)
        {
            switch (style)
            {
                case button_style::primary:
                    return iconOnColor;
                case button_style::secondary:
                    return iconSecondary;
                case button_style::ghost:
                    return iconPrimary;
                case button_style::danger:
                    return iconOnColor;
            }
            return iconPrimary;
        }

        inline void apply_colours()
        {
            ImGuiStyle* style = &ImGui::GetStyle();

            // Text
            style->Colors[ImGuiCol_Text]         = textPrimary;
            style->Colors[ImGuiCol_TextDisabled] = textDisabled;

            // Backgrounds
            style->Colors[ImGuiCol_MenuBarBg]         = background;  // Menu Bar (close buttons etc.)
            style->Colors[ImGuiCol_WindowBg]          = layer01;     // Main windows background
            style->Colors[ImGuiCol_ChildBg]           = background;
            style->Colors[ImGuiCol_FrameBg]           = layer02;
            style->Colors[ImGuiCol_PopupBg]           = layer02;
            style->Colors[ImGuiCol_ScrollbarBg]       = background;
            style->Colors[ImGuiCol_DockingEmptyBg]    = background;
            style->Colors[ImGuiCol_TableRowBg]        = missingColorColor;
            style->Colors[ImGuiCol_TableRowBgAlt]     = missingColorColor;
            style->Colors[ImGuiCol_TableHeaderBg]     = missingColorColor;
            style->Colors[ImGuiCol_TextSelectedBg]    = fieldHover01;
            style->Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.5f);
            style->Colors[ImGuiCol_ModalWindowDimBg]  = ImVec4(0.0f, 0.0f, 0.0f, 0.5f);

            // Tabs are both tabs inside a window
            // and also when windows are docked, they are tabbed
            style->Colors[ImGuiCol_Tab]                 = field02;      // actual tab
            style->Colors[ImGuiCol_TabHovered]          = field01;      // actual tab and dock tab
            style->Colors[ImGuiCol_TabActive]           = field01;      // actual tab and dock tab
            style->Colors[ImGuiCol_TabSelectedOverline] = interactive;  // top line over dock tab
            style->Colors[ImGuiCol_TabSelected]         = field01;      // actual tab and dock tab

            style->Colors[ImGuiCol_TabUnfocused]       = missingColorColor;
            style->Colors[ImGuiCol_TabUnfocusedActive] = missingColorColor;

            style->Colors[ImGuiCol_TabDimmedSelected] = field01;  // selected dock tab
            style->Colors[ImGuiCol_TabDimmed]         = background;

            // Window Titles
            style->Colors[ImGuiCol_TitleBg]          = background;
            style->Colors[ImGuiCol_TitleBgActive]    = background;
            style->Colors[ImGuiCol_TitleBgCollapsed] = background;

            style->Colors[ImGuiCol_FrameBgHovered] = fieldHover01;
            style->Colors[ImGuiCol_FrameBgActive]  = fieldHover01;

            // Borders around elements / tabs
            style->Colors[ImGuiCol_Border]       = borderSubtle00;
            style->Colors[ImGuiCol_BorderShadow] = shadow;

            // Scrollbar
            style->Colors[ImGuiCol_ScrollbarGrab]        = layer03;
            style->Colors[ImGuiCol_ScrollbarGrabHovered] = layerHover03;
            style->Colors[ImGuiCol_ScrollbarGrabActive]  = layerActive03;

            style->Colors[ImGuiCol_CheckMark]        = interactive;
            style->Colors[ImGuiCol_SliderGrab]       = field03;
            style->Colors[ImGuiCol_SliderGrabActive] = fieldHover03;

            // Buttons
            style->Colors[ImGuiCol_Button]        = interactive;
            style->Colors[ImGuiCol_ButtonHovered] = interactiveHover;
            style->Colors[ImGuiCol_ButtonActive]  = interactiveActive;

            // Collapsing Headers
            style->Colors[ImGuiCol_Header]        = layer02;
            style->Colors[ImGuiCol_HeaderHovered] = layerHover02;
            style->Colors[ImGuiCol_HeaderActive]  = layerActive02;

            style->Colors[ImGuiCol_ResizeGrip]        = interactive;
            style->Colors[ImGuiCol_ResizeGripHovered] = interactiveHover;
            style->Colors[ImGuiCol_ResizeGripActive]  = interactiveActive;

            style->Colors[ImGuiCol_PlotLines]            = missingColorColor;
            style->Colors[ImGuiCol_PlotLinesHovered]     = missingColorColor;
            style->Colors[ImGuiCol_PlotHistogram]        = interactive;  // slider
            style->Colors[ImGuiCol_PlotHistogramHovered] = missingColorColor;

            // Separators
            style->Colors[ImGuiCol_Separator]        = borderStrong02;
            style->Colors[ImGuiCol_SeparatorHovered] = borderStrong03;
            style->Colors[ImGuiCol_SeparatorActive]  = borderStrong01;

            style->Colors[ImGuiCol_TableBorderStrong] = borderStrong01;
            style->Colors[ImGuiCol_TableBorderLight]  = borderSubtle01;

            style->Colors[ImGuiCol_DockingPreview] = interactive;
            style->Colors[ImGuiCol_DragDropTarget] = supportInfo;

            style->Colors[ImGuiCol_NavHighlight]          = interactiveActive;
            style->Colors[ImGuiCol_NavWindowingHighlight] = borderInteractive;  // CTRL+TAB and docking previews

            // ImPlot::GetStyle().Colors[ImPlotCol_Fill] = green50;
        }

        inline void apply_styles()
        {
            ImGuiStyle* style = &ImGui::GetStyle();

            style->Alpha         = 1.0f;
            style->DisabledAlpha = 0.5f;

            // Spacing -- everything sits on the 4px scale (with the two 6/10 half-steps
            // called out explicitly in the spec's ItemSpacing / ItemInnerSpacing).
            style->WindowPadding          = insetWindow;
            style->FramePadding           = insetFrame;
            style->CellPadding            = insetFrame;
            style->ItemSpacing            = ImVec2(space12, 10.0f);
            style->ItemInnerSpacing       = ImVec2(space08, 6.0f);
            style->IndentSpacing          = space20;
            style->SeparatorTextPadding   = insetFrame;
            style->DisplayWindowPadding   = insetFrame;
            style->DisplaySafeAreaPadding = insetFrame;
            style->TouchExtraPadding      = insetNone;
            style->ColumnsMinSpacing      = space08;

            // Rounding
            style->WindowRounding    = radiusLg;
            style->ChildRounding     = radiusLg;
            style->PopupRounding     = radiusLg;
            style->FrameRounding     = radiusMd;
            style->GrabRounding      = radiusMd;
            style->TabRounding       = radiusMd;
            style->ScrollbarRounding = 5.0f;

            // Borders -- depth comes from tone and shadow, not outlines. The one
            // hairline permitted is the tab-bar separator, and the separator-text
            // rule that already lives on top of a surface.
            style->WindowBorderSize        = 0.0f;
            style->ChildBorderSize         = 0.0f;
            style->PopupBorderSize         = 0.0f;
            style->FrameBorderSize         = 0.0f;
            style->TabBorderSize           = 0.0f;
            style->TabBarBorderSize        = 1.0f;
            style->SeparatorTextBorderSize = 1.0f;

            // Sizes
            style->WindowMinSize        = ImVec2(100.0f, 100.0f);
            style->ScrollbarSize        = 10.0f;
            style->GrabMinSize          = 12.0f;
            style->DockingSeparatorSize = space04;

            // Alignment
            style->WindowTitleAlign         = ImVec2(0.0f, 0.5f);
            style->WindowMenuButtonPosition = ImGuiDir_Right;
            style->ButtonTextAlign          = ImVec2(0.0f, 0.5f);
            style->SelectableTextAlign      = ImVec2(0.5f, 0.5f);
            style->SeparatorTextAlign       = ImVec2(0.5f, 0.5f);
            style->TableAngledHeadersAngle  = 45.0f;

            style->AntiAliasedLines       = true;
            style->AntiAliasedLinesUseTex = true;
            style->AntiAliasedFill        = true;
        }
    }  // namespace gluten::theme
}  // namespace gluten