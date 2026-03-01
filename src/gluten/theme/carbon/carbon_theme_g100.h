#pragma once

#include "gluten/theme/carbon/carbon_colors.h"
#include "implot.h"

namespace gluten::theme::carbon_g100
{
    // https://github.com/carbon-design-system/carbon/blob/v10/packages/themes/src/next/g100.js

    // Background
    constexpr ImVec4 background              = gray100;  // Keep - darkest base
    constexpr ImVec4 backgroundInverse       = gray10;
    constexpr ImVec4 backgroundBrand         = blue60;
    constexpr ImVec4 backgroundActive        = adjust_alpha(gray50, 0.5f);  // Increased from 0.4f
    constexpr ImVec4 backgroundHover         = adjust_alpha(gray50, 0.2f);  // Increased from 0.16f for better visibility
    constexpr ImVec4 backgroundInverseHover  = gray10Hover;
    constexpr ImVec4 backgroundSelected      = adjust_alpha(gray50, 0.3f);  // Increased from 0.24f
    constexpr ImVec4 backgroundSelectedHover = adjust_alpha(gray50, 0.4f);  // Increased from 0.32f

    // Layer
    // layer-01 - Use for main content area (lighter than sidebar)
    constexpr ImVec4 layer01              = gray90;  // Main content background
    constexpr ImVec4 layerActive01        = gray70;
    constexpr ImVec4 layerHover01         = gray80Hover;  // Changed from gray90Hover for better contrast
    constexpr ImVec4 layerSelected01      = gray80;
    constexpr ImVec4 layerSelectedHover01 = gray70Hover;  // Changed from gray80Hover

    // layer-02 - Use for sidebar/panels (medium depth)
    constexpr ImVec4 layer02              = gray80;  // Sidebar background
    constexpr ImVec4 layerActive02        = gray60;
    constexpr ImVec4 layerHover02         = gray70Hover;  // Changed from gray80Hover
    constexpr ImVec4 layerSelected02      = gray70;
    constexpr ImVec4 layerSelectedHover02 = gray60Hover;  // Changed from gray70Hover

    // layer-03 - Use for elevated elements (cards, modals)
    constexpr ImVec4 layer03              = gray70;  // Elevated/card backgrounds
    constexpr ImVec4 layerActive03        = gray50;
    constexpr ImVec4 layerHover03         = gray60Hover;  // Changed from gray70Hover
    constexpr ImVec4 layerSelected03      = gray60;
    constexpr ImVec4 layerSelectedHover03 = gray50Hover;  // Changed from gray60Hover

    // layer
    constexpr ImVec4 layerSelectedInverse  = gray10;
    constexpr ImVec4 layerSelectedDisabled = gray40;

    // layer-accent-01
    constexpr ImVec4 layerAccent01       = gray80;
    constexpr ImVec4 layerAccentActive01 = gray60;
    constexpr ImVec4 layerAccentHover01  = gray70Hover;  // Changed from gray80Hover

    // layer-accent-02
    constexpr ImVec4 layerAccent02       = gray70;
    constexpr ImVec4 layerAccentActive02 = gray50;
    constexpr ImVec4 layerAccentHover02  = gray60Hover;  // Changed from gray70Hover

    // layer-accent-03
    constexpr ImVec4 layerAccent03       = gray60;
    constexpr ImVec4 layerAccentActive03 = gray80;
    constexpr ImVec4 layerAccentHover03  = gray50Hover;  // Changed from gray60Hover

    // Field
    // field-01
    constexpr ImVec4 field01      = gray90;
    constexpr ImVec4 fieldHover01 = gray80Hover;  // Changed from gray90Hover

    // field-02
    constexpr ImVec4 field02      = gray80;
    constexpr ImVec4 fieldHover02 = gray70Hover;  // Changed from gray80Hover

    // field-03
    constexpr ImVec4 field03      = gray70;
    constexpr ImVec4 fieldHover03 = gray60Hover;  // Changed from gray70Hover

    // Border
    // border-subtle-00
    constexpr ImVec4 borderSubtle00 = gray80;

    // border-subtle-01
    constexpr ImVec4 borderSubtle01         = gray70;
    constexpr ImVec4 borderSubtleSelected01 = gray60;

    // border-subtle-02
    constexpr ImVec4 borderSubtle02         = gray60;
    constexpr ImVec4 borderSubtleSelected02 = gray50;

    // border-subtle-03
    constexpr ImVec4 borderSubtle03         = gray50;  // Changed from gray60 for more contrast
    constexpr ImVec4 borderSubtleSelected03 = gray40;  // Changed from gray50

    // border-strong
    constexpr ImVec4 borderStrong01 = gray60;
    constexpr ImVec4 borderStrong02 = gray50;
    constexpr ImVec4 borderStrong03 = gray30;  // Changed from gray40 for stronger borders

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
    constexpr ImVec4 textSecondary       = gray20;  
    constexpr ImVec4 textPlaceholder     = adjust_alpha(textPrimary, 0.4f);
    constexpr ImVec4 textHelper          = gray50;  // Changed from gray40
    constexpr ImVec4 textError           = red40;
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
    constexpr ImVec4 iconSecondary       = gray40;  // Changed from gray30 to match textSecondary
    constexpr ImVec4 iconInverse         = gray100;
    constexpr ImVec4 iconOnColor         = white;
    constexpr ImVec4 iconOnColorDisabled = adjust_alpha(iconOnColor, 0.25f);
    constexpr ImVec4 iconDisabled        = adjust_alpha(iconPrimary, 0.25f);
    constexpr ImVec4 iconInteractive     = white;

    // Support
    constexpr ImVec4 supportError            = red50;
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
    constexpr ImVec4 interactive       = blue50;
    constexpr ImVec4 interactiveHover  = blue40;
    constexpr ImVec4 interactiveActive = blue60;
    constexpr ImVec4 highlight         = blue80;
    constexpr ImVec4 overlay           = adjust_alpha(black, 0.65f);
    constexpr ImVec4 toggleOff         = gray60;
    constexpr ImVec4 shadow            = adjust_alpha(black, 0.8f);

    constexpr ImVec4 missingColorColor = magenta50;

    inline void apply_colours()
    {
        gluten::theme::textPrimary  = textPrimary;
        gluten::theme::textDisabled = textDisabled;
        gluten::theme::background   = background;
        gluten::theme::layer01      = layer01;
        gluten::theme::layer02      = layer02;
        gluten::theme::layer03      = layer03;
        gluten::theme::field01      = field01;
        gluten::theme::field02      = field02;
        gluten::theme::field03      = field03;
        gluten::theme::fieldHover01 = fieldHover01;
        gluten::theme::fieldHover02 = fieldHover02;
        gluten::theme::fieldHover03 = fieldHover03;
    }

    inline void apply_styles()
    {
        gluten::theme::padding = padding;
        gluten::theme::noPadding = noPadding;
        gluten::theme::rounding  = rounding;
    }
}  // namespace gluten::theme