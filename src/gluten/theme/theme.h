#pragma once

#include "imgui.h"
#include "gluten/pch.h"

namespace gluten::theme
{
    constexpr auto invalidPrefab = IM_COL32(222, 43, 43, 255);

    inline ImVec4 color_with_value(const ImColor& color, float value)
    {
        const ImVec4& colRow = color.Value;
        float hue, sat, val;
        ImGui::ColorConvertRGBtoHSV(colRow.x, colRow.y, colRow.z, hue, sat, val);
        return ImColor::HSV(hue, sat, std::min(value, 1.0f));
    }

    inline ImVec4 color_with_saturation(const ImColor& color, float saturation)
    {
        const ImVec4& colRow = color.Value;
        float hue, sat, val;
        ImGui::ColorConvertRGBtoHSV(colRow.x, colRow.y, colRow.z, hue, sat, val);
        return ImColor::HSV(hue, std::min(saturation, 1.0f), val);
    }

    inline ImVec4 color_with_hue(const ImColor& color, float hue)
    {
        const ImVec4& colRow = color.Value;
        float h, s, v;
        ImGui::ColorConvertRGBtoHSV(colRow.x, colRow.y, colRow.z, h, s, v);
        return ImColor::HSV(std::min(hue, 1.0f), s, v);
    }

    inline ImVec4 color_with_multiplied_value(const ImColor& color, float multiplier)
    {
        const ImVec4& colRow = color.Value;
        float hue, sat, val;
        ImGui::ColorConvertRGBtoHSV(colRow.x, colRow.y, colRow.z, hue, sat, val);
        return ImColor::HSV(hue, sat, std::min(val * multiplier, 1.0f));
    }

    inline ImVec4 color_with_multiplied_saturation(const ImColor& color, float multiplier)
    {
        const ImVec4& colRow = color.Value;
        float hue, sat, val;
        ImGui::ColorConvertRGBtoHSV(colRow.x, colRow.y, colRow.z, hue, sat, val);
        return ImColor::HSV(hue, std::min(sat * multiplier, 1.0f), val);
    }

    inline ImU32 color_with_multiplied_hue(const ImColor& color, float multiplier)
    {
        const ImVec4& colRow = color.Value;
        float hue, sat, val;
        ImGui::ColorConvertRGBtoHSV(colRow.x, colRow.y, colRow.z, hue, sat, val);
        return ImColor::HSV(std::min(hue * multiplier, 1.0f), sat, val);
    }

    inline ImVec4 color_with_added_value(const ImColor& color, float delta)
    {
        const ImVec4& colRow = color.Value;
        float hue, sat, val;
        ImGui::ColorConvertRGBtoHSV(colRow.x, colRow.y, colRow.z, hue, sat, val);
        return ImColor::HSV(hue, sat, std::min(val + delta, 1.0f));
    }

    inline ImVec4 color_with_added_saturation(const ImColor& color, float delta)
    {
        const ImVec4& colRow = color.Value;
        float hue, sat, val;
        ImGui::ColorConvertRGBtoHSV(colRow.x, colRow.y, colRow.z, hue, sat, val);
        return ImColor::HSV(hue, std::min(sat + delta, 1.0f), val);
    }

    inline ImU32 color_with_added_hue(const ImColor& color, float delta)
    {
        const ImVec4& colRow = color.Value;
        float hue, sat, val;
        ImGui::ColorConvertRGBtoHSV(colRow.x, colRow.y, colRow.z, hue, sat, val);
        return ImColor::HSV(std::min(hue + delta, 1.0f), sat, val);
    }

    consteval ImVec4 adjust_alpha(const ImVec4& color, const float& alpha)
    {
        return ImVec4(color.x, color.y, color.z, alpha);
    }

    consteval ImVec4 hex_to_imgui_imvec4(const unsigned long& hex)
    {
        const float s = 1.0f / 255.0f;

        // The RGB order is backwards here -> BGR
        // Also, set alpha to 1 as the carbon hex values don't hold alpha and therefore
        // everything would get set to 0
        return ImVec4(((hex >> IM_COL32_B_SHIFT) & 0xFF) * s, ((hex >> IM_COL32_G_SHIFT) & 0xFF) * s,
                      ((hex >> IM_COL32_R_SHIFT) & 0xFF) * s, 1.0f);
    }

    extern ImVec4 white;
    extern ImVec4 black;

    // Background
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
    extern ImVec4 highlight;
    extern ImVec4 overlay;
    extern ImVec4 toggleOff;
    extern ImVec4 shadow;

    extern ImVec4 missingColorColor;

    extern float appTitlebarHeightMultiplier;
    extern float padding;
    extern ImVec2 noPadding;
    extern ImVec2 paddingVec;
    extern ImVec2 verticalPaddingVec;

    extern float rounding;
    extern float largerRounding;
    extern float largestRounding;
    extern float noRounding;

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
        style->Colors[ImGuiCol_NavWindowingDimBg] = layer01;
        style->Colors[ImGuiCol_ModalWindowDimBg]  = layer01;

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

        style->WindowPadding = ImVec2(padding, 0.0f);
        style->FramePadding  = ImVec2(padding, padding * 1.5f);

        style->WindowRounding    = rounding;
        style->ChildRounding     = 0.0f;
        style->PopupRounding     = rounding;
        style->FrameRounding     = rounding;
        style->ScrollbarRounding = rounding;
        style->GrabRounding      = rounding;
        style->TabRounding       = rounding;

        style->SeparatorTextPadding;
        style->DisplayWindowPadding = paddingVec;
        style->DisplaySafeAreaPadding;
        style->CellPadding       = paddingVec;
        style->TouchExtraPadding = ImVec2(0, 0);

        style->WindowBorderSize         = 2.0f;
        style->WindowMinSize            = ImVec2(100, 100);
        style->WindowTitleAlign         = ImVec2(0.1f, 0.5f);
        style->WindowMenuButtonPosition = ImGuiDir_Right;
        style->ChildBorderSize          = 2.0f;
        style->PopupBorderSize          = 0.0f;
        style->FrameBorderSize          = 0.0f;
        style->ItemSpacing              = ImVec2(6, 8);
        style->ItemInnerSpacing         = ImVec2(0, 0);
        style->IndentSpacing            = 24.0f;
        style->ColumnsMinSpacing        = 10.0f;
        style->ScrollbarSize            = 18.0f;
        style->GrabMinSize              = 12.0f;
        style->LogSliderDeadzone;
        style->TabBorderSize           = 0.0f;
        style->TabBarBorderSize        = 1.0f;
        style->TableAngledHeadersAngle = 45.0f;
        style->ColorButtonPosition;
        style->ButtonTextAlign         = ImVec2(0.0f, 0.5f);
        style->SelectableTextAlign     = ImVec2(0.5f, 0.5f);
        style->SeparatorTextBorderSize = 1.0f;
        style->SeparatorTextAlign      = ImVec2(0.5f, 0.5f);
        style->DockingSeparatorSize    = 4.0f;
        style->MouseCursorScale;
        style->AntiAliasedLines;
        style->AntiAliasedLinesUseTex;
        style->AntiAliasedFill;
        style->CurveTessellationTol;
        style->CircleTessellationMaxError;
    }
}  // namespace gluten::theme
