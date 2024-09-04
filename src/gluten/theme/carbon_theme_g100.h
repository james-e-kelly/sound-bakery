#pragma once

#include "gluten/theme/carbon_colors.h"

namespace gluten::theme::carbon_g100
{
    // https://github.com/carbon-design-system/carbon/blob/v10/packages/themes/src/next/g90.js

    // Background
    constexpr ImVec4 background              = gray90;
    constexpr ImVec4 backgroundInverse       = gray10;
    constexpr ImVec4 backgroundBrand         = blue60;
    constexpr ImVec4 backgroundActive        = adjust_alpha(gray50, 0.4);
    constexpr ImVec4 backgroundHover         = adjust_alpha(gray50, 0.16);
    constexpr ImVec4 backgroundInverseHover  = gray10Hover;
    constexpr ImVec4 backgroundSelected      = adjust_alpha(gray50, 0.24);
    constexpr ImVec4 backgroundSelectedHover = adjust_alpha(gray50, 0.32);

    // Layer
    // layer-01
    constexpr ImVec4 layer01              = gray80;
    constexpr ImVec4 layerActive01        = gray60;
    constexpr ImVec4 layerHover01         = gray80Hover;
    constexpr ImVec4 layerSelected01      = gray70;
    constexpr ImVec4 layerSelectedHover01 = gray70Hover;

    // layer-02
    constexpr ImVec4 layer02              = gray70;
    constexpr ImVec4 layerActive02        = gray50;
    constexpr ImVec4 layerHover02         = gray70Hover;
    constexpr ImVec4 layerSelected02      = gray60;
    constexpr ImVec4 layerSelectedHover02 = gray60Hover;

    // layer-03
    constexpr ImVec4 layer03              = gray60;
    constexpr ImVec4 layerActive03        = gray80;
    constexpr ImVec4 layerHover03         = gray60Hover;
    constexpr ImVec4 layerSelected03      = gray50;
    constexpr ImVec4 layerSelectedHover03 = gray50Hover;

    // layer
    constexpr ImVec4 layerSelectedInverse  = gray10;
    constexpr ImVec4 layerSelectedDisabled = gray40;

    // layer-accent-01
    constexpr ImVec4 layerAccent01       = gray70;
    constexpr ImVec4 layerAccentActive01 = gray50;
    constexpr ImVec4 layerAccentHover01  = gray70Hover;

    // layer-accent-02
    constexpr ImVec4 layerAccent02       = gray60;
    constexpr ImVec4 layerAccentActive02 = gray80;
    constexpr ImVec4 layerAccentHover02  = gray60Hover;

    // layer-accent-03
    constexpr ImVec4 layerAccent03       = gray50;
    constexpr ImVec4 layerAccentActive03 = gray70;
    constexpr ImVec4 layerAccentHover03  = gray50Hover;

    // Field
    // field-01
    constexpr ImVec4 field01      = gray80;
    constexpr ImVec4 fieldHover01 = gray80Hover;

    // field-02
    constexpr ImVec4 field02      = gray70;
    constexpr ImVec4 fieldHover02 = gray70Hover;

    // field-03
    constexpr ImVec4 field03      = gray60;
    constexpr ImVec4 fieldHover03 = gray60Hover;

    // Border
    // border-subtle-00
    constexpr ImVec4 borderSubtle00 = gray70;

    // border-subtle-01
    constexpr ImVec4 borderSubtle01         = gray70;
    constexpr ImVec4 borderSubtleSelected01 = gray60;

    // border-subtle-02
    constexpr ImVec4 borderSubtle02         = gray60;
    constexpr ImVec4 borderSubtleSelected02 = gray50;

    // border-subtle-03
    constexpr ImVec4 borderSubtle03         = gray50;
    constexpr ImVec4 borderSubtleSelected03 = gray40;

    // border-strong
    constexpr ImVec4 borderStrong01 = gray50;
    constexpr ImVec4 borderStrong02 = gray40;
    constexpr ImVec4 borderStrong03 = gray30;

    // border-tile
    constexpr ImVec4 borderTile01 = gray70;
    constexpr ImVec4 borderTile02 = gray60;
    constexpr ImVec4 borderTile03 = gray50;

    // border-inverse
    constexpr ImVec4 borderInverse = gray10;

    // border-interactive
    constexpr ImVec4 borderInteractive = blue50;

    // border
    constexpr ImVec4 borderDisabled = adjust_alpha(gray50, 0.5f);

    // Text
    constexpr ImVec4 textPrimary         = gray10;
    constexpr ImVec4 textSecondary       = gray30;
    constexpr ImVec4 textPlaceholder     = adjust_alpha(textPrimary, 0.4f);
    constexpr ImVec4 textHelper          = gray30;
    constexpr ImVec4 textError           = red30;
    constexpr ImVec4 textInverse         = gray100;
    constexpr ImVec4 textOnColor         = white;
    constexpr ImVec4 textOnColorDisabled = adjust_alpha(textOnColor, 0.25f);
    constexpr ImVec4 textDisabled        = adjust_alpha(textPrimary, 0.25f);

    // Link
    constexpr ImVec4 linkPrimary       = blue40;
    constexpr ImVec4 linkPrimaryHover  = blue30;
    constexpr ImVec4 linkSecondary     = blue30;
    constexpr ImVec4 linkInverse       = blue60;
    constexpr ImVec4 linkVisited       = purple40;
    constexpr ImVec4 linkInverseActive = gray100;
    constexpr ImVec4 linkInverseHover  = blue70;

    // Icon
    constexpr ImVec4 iconPrimary         = gray10;
    constexpr ImVec4 iconSecondary       = gray30;
    constexpr ImVec4 iconInverse         = gray100;
    constexpr ImVec4 iconOnColor         = white;
    constexpr ImVec4 iconOnColorDisabled = adjust_alpha(iconOnColor, 0.25f);
    constexpr ImVec4 iconDisabled        = adjust_alpha(iconPrimary, 0.25f);
    constexpr ImVec4 iconInteractive     = white;

    // Support
    constexpr ImVec4 supportError            = red40;
    constexpr ImVec4 supportSuccess          = green40;
    constexpr ImVec4 supportWarning          = yellow30;
    constexpr ImVec4 supportInfo             = blue50;
    constexpr ImVec4 supportErrorInverse     = red60;
    constexpr ImVec4 supportSuccessInverse   = green50;
    constexpr ImVec4 supportWarningInverse   = yellow30;
    constexpr ImVec4 supportInfoInverse      = blue70;
    constexpr ImVec4 supportCautionMinor     = yellow30;
    constexpr ImVec4 supportCautionMajor     = orange40;
    constexpr ImVec4 supportCautionUndefined = purple50;

    // Focus
    constexpr ImVec4 focus        = white;
    constexpr ImVec4 focusInset   = gray100;
    constexpr ImVec4 focusInverse = blue60;

    // Misc
    constexpr ImVec4 interactive = blue50;
    constexpr ImVec4 interactiveHover = blue40;
    constexpr ImVec4 interactiveActive = blue60;
    constexpr ImVec4 highlight   = blue70;
    constexpr ImVec4 overlay     = adjust_alpha(black, 0.65f);
    constexpr ImVec4 toggleOff   = gray60;
    constexpr ImVec4 shadow      = adjust_alpha(black, 0.8f);

    constexpr ImVec4 missingColorColor = magenta50;

    inline void apply_theme() 
    { 
        ImGuiStyle* style = &ImGui::GetStyle();

        // Text
        style->Colors[ImGuiCol_Text]         = textPrimary;
        style->Colors[ImGuiCol_TextDisabled] = textDisabled;

        // Backgrounds
        style->Colors[ImGuiCol_MenuBarBg] = background;    // Menu Bar (close buttons etc.)
        style->Colors[ImGuiCol_WindowBg]  = background;    // Main windows background
        style->Colors[ImGuiCol_ChildBg]   = layer01;
        style->Colors[ImGuiCol_FrameBg]   = layer02;
        style->Colors[ImGuiCol_PopupBg]        = layer02;
        style->Colors[ImGuiCol_ScrollbarBg]    = layer02;
        style->Colors[ImGuiCol_DockingEmptyBg] = background;
        style->Colors[ImGuiCol_TableRowBg]     = missingColorColor;
        style->Colors[ImGuiCol_TableRowBgAlt]  = missingColorColor;
        style->Colors[ImGuiCol_TableHeaderBg]  = missingColorColor;
        style->Colors[ImGuiCol_TextSelectedBg] = fieldHover01;
        style->Colors[ImGuiCol_NavWindowingDimBg] = layer01;
        style->Colors[ImGuiCol_ModalWindowDimBg]  = layer01;

        // Tabs are both tabs inside a window
        // and also when windows are docked, they are tabbed
        style->Colors[ImGuiCol_Tab]                 = field02;              // actual tab
        style->Colors[ImGuiCol_TabHovered]          = field01;              // actual tab and dock tab
        style->Colors[ImGuiCol_TabActive]           = background;           // actual tab and dock tab
        style->Colors[ImGuiCol_TabSelectedOverline] = interactive;          // top line over dock tab
        style->Colors[ImGuiCol_TabSelected]         = background;           // actual tab and dock tab

        style->Colors[ImGuiCol_TabUnfocused]       = missingColorColor;
        style->Colors[ImGuiCol_TabUnfocusedActive] = missingColorColor;

        style->Colors[ImGuiCol_TabDimmedSelected] = background;                // selected dock tab
        style->Colors[ImGuiCol_TabDimmed]         = field01;

        // Window Titles
        style->Colors[ImGuiCol_TitleBg]          = background;
        style->Colors[ImGuiCol_TitleBgActive]    = background;
        style->Colors[ImGuiCol_TitleBgCollapsed] = background;

        style->Colors[ImGuiCol_FrameBgHovered]   = fieldHover01;
        style->Colors[ImGuiCol_FrameBgActive]  = fieldHover01;

        // Borders around elements / tabs
        style->Colors[ImGuiCol_Border]       = borderStrong01;
        style->Colors[ImGuiCol_BorderShadow] = ImVec4(0,0,0,0);

        // Scrollbar
        style->Colors[ImGuiCol_ScrollbarGrab]        = layer03;
        style->Colors[ImGuiCol_ScrollbarGrabHovered] = layerHover03;
        style->Colors[ImGuiCol_ScrollbarGrabActive]  = layerActive03;

        style->Colors[ImGuiCol_CheckMark]        = missingColorColor;
        style->Colors[ImGuiCol_SliderGrab]       = field03;
        style->Colors[ImGuiCol_SliderGrabActive] = fieldHover03;

        // Buttons
        style->Colors[ImGuiCol_Button]        = interactive;
        style->Colors[ImGuiCol_ButtonHovered] = interactiveHover;
        style->Colors[ImGuiCol_ButtonActive]  = interactiveActive;

        // Collapsing Headers
        style->Colors[ImGuiCol_Header]        = background;
        style->Colors[ImGuiCol_HeaderHovered] = backgroundHover;
        style->Colors[ImGuiCol_HeaderActive]  = backgroundActive;

        style->Colors[ImGuiCol_ResizeGrip]        = interactive;
        style->Colors[ImGuiCol_ResizeGripHovered] = interactiveHover;
        style->Colors[ImGuiCol_ResizeGripActive]  = interactiveActive;

        style->Colors[ImGuiCol_PlotLines]            = missingColorColor;
        style->Colors[ImGuiCol_PlotLinesHovered]     = missingColorColor;
        style->Colors[ImGuiCol_PlotHistogram]        = missingColorColor;
        style->Colors[ImGuiCol_PlotHistogramHovered] = missingColorColor;

        // Separators
        style->Colors[ImGuiCol_Separator]        = background;
        style->Colors[ImGuiCol_SeparatorHovered] = backgroundHover;
        style->Colors[ImGuiCol_SeparatorActive]  = backgroundActive;

        style->Colors[ImGuiCol_TableBorderStrong] = borderStrong01;
        style->Colors[ImGuiCol_TableBorderLight]  = borderSubtle01;

        style->Colors[ImGuiCol_DockingPreview] = interactive;
        style->Colors[ImGuiCol_DragDropTarget] = missingColorColor;

        style->Colors[ImGuiCol_NavHighlight]          = missingColorColor;
        style->Colors[ImGuiCol_NavWindowingHighlight] = borderInteractive;  // CTRL+TAB and docking previews

        // IMPLOT

        ImPlotStyle& plotStyle = ImPlot::GetStyle();

        plotStyle.Colors[ImPlotCol_Fill] = supportSuccess;
    }
}