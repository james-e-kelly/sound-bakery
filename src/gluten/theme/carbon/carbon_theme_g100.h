#pragma once

#include "gluten/theme/carbon/carbon_colors.h"
#include "implot.h"

namespace gluten::theme::carbon_g100
{
    // https://github.com/carbon-design-system/carbon/blob/v10/packages/themes/src/next/g100.js

    // Background
    static ImVec4 background              = gray100;  // Keep - darkest base
    static ImVec4 backgroundInverse       = gray10;
    static ImVec4 backgroundBrand         = blue60;
    static ImVec4 backgroundActive        = adjust_alpha(gray50, 0.5f);  // Increased from 0.4f
    static ImVec4 backgroundHover         = adjust_alpha(gray50, 0.2f);  // Increased from 0.16f for better visibility
    static ImVec4 backgroundInverseHover  = gray10Hover;
    static ImVec4 backgroundSelected      = adjust_alpha(gray50, 0.3f);  // Increased from 0.24f
    static ImVec4 backgroundSelectedHover = adjust_alpha(gray50, 0.4f);  // Increased from 0.32f

    // Layer
    // layer-01 - Use for main content area (lighter than sidebar)
    static ImVec4 layer01              = gray90;  // Main content background
    static ImVec4 layerActive01        = gray70;
    static ImVec4 layerHover01         = gray80Hover;  // Changed from gray90Hover for better contrast
    static ImVec4 layerSelected01      = gray80;
    static ImVec4 layerSelectedHover01 = gray70Hover;  // Changed from gray80Hover

    // layer-02 - Use for sidebar/panels (medium depth)
    static ImVec4 layer02              = gray80;  // Sidebar background
    static ImVec4 layerActive02        = gray60;
    static ImVec4 layerHover02         = gray70Hover;  // Changed from gray80Hover
    static ImVec4 layerSelected02      = gray70;
    static ImVec4 layerSelectedHover02 = gray60Hover;  // Changed from gray70Hover

    // layer-03 - Use for elevated elements (cards, modals)
    static ImVec4 layer03              = gray70;  // Elevated/card backgrounds
    static ImVec4 layerActive03        = gray50;
    static ImVec4 layerHover03         = gray60Hover;  // Changed from gray70Hover
    static ImVec4 layerSelected03      = gray60;
    static ImVec4 layerSelectedHover03 = gray50Hover;  // Changed from gray60Hover

    // layer
    static ImVec4 layerSelectedInverse  = gray10;
    static ImVec4 layerSelectedDisabled = gray40;

    // layer-accent-01
    static ImVec4 layerAccent01       = gray80;
    static ImVec4 layerAccentActive01 = gray60;
    static ImVec4 layerAccentHover01  = gray70Hover;  // Changed from gray80Hover

    // layer-accent-02
    static ImVec4 layerAccent02       = gray70;
    static ImVec4 layerAccentActive02 = gray50;
    static ImVec4 layerAccentHover02  = gray60Hover;  // Changed from gray70Hover

    // layer-accent-03
    static ImVec4 layerAccent03       = gray60;
    static ImVec4 layerAccentActive03 = gray80;
    static ImVec4 layerAccentHover03  = gray50Hover;  // Changed from gray60Hover

    // Field
    // field-01
    static ImVec4 field01      = gray90;
    static ImVec4 fieldHover01 = gray80Hover;  // Changed from gray90Hover

    // field-02
    static ImVec4 field02      = gray80;
    static ImVec4 fieldHover02 = gray70Hover;  // Changed from gray80Hover

    // field-03
    static ImVec4 field03      = gray70;
    static ImVec4 fieldHover03 = gray60Hover;  // Changed from gray70Hover

    // Border
    // border-subtle-00
    static ImVec4 borderSubtle00 = gray80;

    // border-subtle-01
    static ImVec4 borderSubtle01         = gray70;
    static ImVec4 borderSubtleSelected01 = gray60;

    // border-subtle-02
    static ImVec4 borderSubtle02         = gray60;
    static ImVec4 borderSubtleSelected02 = gray50;

    // border-subtle-03
    static ImVec4 borderSubtle03         = gray50;  // Changed from gray60 for more contrast
    static ImVec4 borderSubtleSelected03 = gray40;  // Changed from gray50

    // border-strong
    static ImVec4 borderStrong01 = gray60;
    static ImVec4 borderStrong02 = gray50;
    static ImVec4 borderStrong03 = gray30;  // Changed from gray40 for stronger borders

    // border-tile
    static ImVec4 borderTile01 = gray70;
    static ImVec4 borderTile02 = gray60;
    static ImVec4 borderTile03 = gray50;

    // border-inverse
    static ImVec4 borderInverse = gray10;

    // border-interactive
    static ImVec4 borderInteractive = blue50;

    // border
    static ImVec4 borderDisabled = adjust_alpha(gray50, 0.5f);

    // Text
    static ImVec4 textPrimary         = gray10;
    static ImVec4 textSecondary       = gray20;  
    static ImVec4 textPlaceholder     = adjust_alpha(textPrimary, 0.4f);
    static ImVec4 textHelper          = gray50;  // Changed from gray40
    static ImVec4 textError           = red40;
    static ImVec4 textInverse         = gray100;
    static ImVec4 textOnColor         = white;
    static ImVec4 textOnColorDisabled = adjust_alpha(textOnColor, 0.25f);
    static ImVec4 textDisabled        = adjust_alpha(textPrimary, 0.25f);

    // Link
    static ImVec4 linkPrimary       = blue40;
    static ImVec4 linkPrimaryHover  = blue30;
    static ImVec4 linkSecondary     = blue30;
    static ImVec4 linkInverse       = blue60;
    static ImVec4 linkVisited       = purple40;
    static ImVec4 linkInverseActive = gray100;
    static ImVec4 linkInverseHover  = blue70;

    // Icon
    static ImVec4 iconPrimary         = gray10;
    static ImVec4 iconSecondary       = gray40;  // Changed from gray30 to match textSecondary
    static ImVec4 iconInverse         = gray100;
    static ImVec4 iconOnColor         = white;
    static ImVec4 iconOnColorDisabled = adjust_alpha(iconOnColor, 0.25f);
    static ImVec4 iconDisabled        = adjust_alpha(iconPrimary, 0.25f);
    static ImVec4 iconInteractive     = white;

    // Support
    static ImVec4 supportError            = red50;
    static ImVec4 supportSuccess          = green40;
    static ImVec4 supportWarning          = yellow30;
    static ImVec4 supportInfo             = blue50;
    static ImVec4 supportErrorInverse     = red60;
    static ImVec4 supportSuccessInverse   = green50;
    static ImVec4 supportWarningInverse   = yellow30;
    static ImVec4 supportInfoInverse      = blue70;
    static ImVec4 supportCautionMinor     = yellow30;
    static ImVec4 supportCautionMajor     = orange40;
    static ImVec4 supportCautionUndefined = purple50;

    // Focus
    static ImVec4 focus        = white;
    static ImVec4 focusInset   = gray100;
    static ImVec4 focusInverse = blue60;

    // Misc
    static ImVec4 interactive       = blue50;
    static ImVec4 interactiveHover  = blue40;
    static ImVec4 interactiveActive = blue60;
    static ImVec4 highlight         = blue80;
    static ImVec4 overlay           = adjust_alpha(black, 0.65f);
    static ImVec4 toggleOff         = gray60;
    static ImVec4 shadow            = adjust_alpha(black, 0.8f);

    static ImVec4 missingColorColor = magenta50;

    inline void apply_colours()
    {
        gluten::theme::background           = background;
        gluten::theme::layer01              = layer01;
        gluten::theme::layer02              = layer02;
        gluten::theme::layer03              = layer03;
        gluten::theme::layerHover01         = layerHover01;
        gluten::theme::layerHover02         = layerHover02;
        gluten::theme::layerHover03         = layerHover03;
        gluten::theme::layerActive01        = layerActive01;
        gluten::theme::layerActive02        = layerActive02;
        gluten::theme::layerActive03        = layerActive03;
        gluten::theme::backgroundHover      = backgroundHover;
        gluten::theme::backgroundActive     = backgroundActive;
        gluten::theme::field01              = field01;
        gluten::theme::field02              = field02;
        gluten::theme::field03              = field03;
        gluten::theme::fieldHover01         = fieldHover01;
        gluten::theme::fieldHover02         = fieldHover02;
        gluten::theme::fieldHover03         = fieldHover03;
        gluten::theme::layerAccent01        = layerAccent01;
        gluten::theme::layerAccent02        = layerAccent02;
        gluten::theme::layerAccent03        = layerAccent03;
        gluten::theme::layerAccentHover01   = layerAccentHover01;
        gluten::theme::layerAccentHover02   = layerAccentHover02;
        gluten::theme::layerAccentHover03   = layerAccentHover03;
        gluten::theme::layerAccentActive01  = layerAccentActive01;
        gluten::theme::layerAccentActive02  = layerAccentActive02;
        gluten::theme::layerAccentActive03  = layerAccentActive03;
        gluten::theme::borderSubtle00       = borderSubtle00;
        gluten::theme::borderSubtle01       = borderSubtle01;
        gluten::theme::borderSubtle02       = borderSubtle02;
        gluten::theme::borderSubtle03       = borderSubtle03;
        gluten::theme::borderStrong01       = borderStrong01;
        gluten::theme::borderStrong02       = borderStrong02;
        gluten::theme::borderStrong03       = borderStrong03;
        gluten::theme::borderInteractive    = borderInteractive;
        gluten::theme::textPrimary          = textPrimary;
        gluten::theme::textSecondary        = textSecondary;
        gluten::theme::textDisabled         = textDisabled;
        gluten::theme::supportError         = supportError;
        gluten::theme::supportSuccess       = supportSuccess;
        gluten::theme::supportWarning       = supportWarning;
        gluten::theme::interactive          = interactive;
        gluten::theme::interactiveHover     = interactiveHover;
        gluten::theme::interactiveActive    = interactiveActive;
    }

    inline void apply_styles()
    {
        gluten::theme::padding = padding;
        gluten::theme::noPadding = noPadding;
        gluten::theme::rounding  = rounding;
    }
}  // namespace gluten::theme