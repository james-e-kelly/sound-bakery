#pragma once

#include "gluten/theme/theme.h"

namespace gluten::theme::things
{
    constexpr ImVec4 base00                     = make_hsv(220 / 360.f, 18 / 100.0f, 11 / 100.f).Value;
    constexpr ImVec4 darkModeBackgroundColor    = make_hsv(220 / 360.f, 18 / 100.0f, 11 / 100.f).Value;
    constexpr ImVec4 lightModeBackgroundColor   = make_hsv(220 / 360.f, 4 / 100.0f, 97 / 100.f).Value;

    constexpr ImVec4 blue                       = hex_to_imgui_imvec4(0x2e80f2);
    constexpr ImVec4 pink                       = hex_to_imgui_imvec4(0xff82b2);
    constexpr ImVec4 green                      = hex_to_imgui_imvec4(0x47e83e);
    constexpr ImVec4 yellow                     = hex_to_imgui_imvec4(0xe5b567);
    constexpr ImVec4 orange                     = hex_to_imgui_imvec4(0xe87d3e);
    constexpr ImVec4 red                        = hex_to_imgui_imvec4(0xe83e3e);
    constexpr ImVec4 purple                     = hex_to_imgui_imvec4(0x9e86c8);

    // Dark mode steps — layers rise in value to convey elevation
    constexpr float hoverValueStep              = 0.08f;
    constexpr float activeValueStep             = 0.1f;
    constexpr float nextLayerValueStep          = 0.035f;
    constexpr float nextLayerSaturationStep     = 0.9f;
    constexpr float prevLayerValueStep          = -0.035f;
    constexpr float prevLayerSaturationStep     = 1.1f;
    constexpr float fieldSaturationStep         = 1.15f;
    constexpr float accentLayerValueStep        = 0.06f;
    constexpr float accentLayerSaturationStep   = 1.10f;

    // Light mode steps — surfaces stay nearly flat; depth via borders/shadow
    constexpr float lightHoverValueStep             = 0.04f;
    constexpr float lightActiveValueStep            = 0.06f;
    constexpr float lightNextLayerValueStep         = 0.01f;
    constexpr float lightPrevLayerValueStep         = -0.01f;
    constexpr float lightAccentLayerValueStep       = 0.025f;

    constexpr auto create_hover_color(const ImVec4& color, bool darkMode) -> ImVec4
    {
        const float step = darkMode ? hoverValueStep : lightHoverValueStep;
        return color_with_added_value(color, step * (darkMode ? 1.0f : -1.0f));
    }

    constexpr auto create_active_color(const ImVec4& color, bool darkMode) -> ImVec4
    {
        const float step = darkMode ? activeValueStep : lightActiveValueStep;
        return color_with_added_value(color, step * (darkMode ? 1.0f : -1.0f));
    }

    constexpr auto create_next_layer(const ImVec4& color, bool darkMode) -> ImVec4
    {
        const float valStep = darkMode ? nextLayerValueStep : lightNextLayerValueStep;
        const float satStep = darkMode ? nextLayerSaturationStep : 1.0f;
        return color_with_multiplied_saturation(color_with_added_value(color, valStep * (darkMode ? 1.0f : -1.0f)), satStep);
    }

    constexpr auto create_prev_layer(const ImVec4& color, bool darkMode) -> ImVec4
    {
        const float valStep = darkMode ? prevLayerValueStep : lightPrevLayerValueStep;
        const float satStep = darkMode ? prevLayerSaturationStep : 1.0f;
        return color_with_multiplied_saturation(color_with_added_value(color, valStep * (darkMode ? 1.0f : -1.0f)), satStep);
    }

    constexpr auto create_field_color(const ImVec4& color, bool darkMode) -> ImVec4
    {
        return color_with_multiplied_saturation(color, fieldSaturationStep);
    }

    constexpr auto create_layer_accent(const ImVec4& color, bool darkMode) -> ImVec4
    {
        const float valStep = darkMode ? accentLayerValueStep : lightAccentLayerValueStep;
        return color_with_multiplied_saturation(color_with_added_value(color, valStep * (darkMode ? 1.0f : -1.0f)), accentLayerSaturationStep);
    }

    inline void apply_colours(ImVec4 backgroundColor, bool darkMode)
    {
        gluten::theme::backgroundLow                = create_prev_layer(backgroundColor, darkMode);
        gluten::theme::background                   = backgroundColor;
        gluten::theme::layer01                      = create_next_layer(gluten::theme::background, darkMode);
        gluten::theme::layer02                      = create_next_layer(gluten::theme::layer01, darkMode);
        gluten::theme::layer03                      = create_next_layer(gluten::theme::layer02, darkMode);

        gluten::theme::layerHover01                 = create_hover_color(gluten::theme::layer01, darkMode);
        gluten::theme::layerHover02                 = create_hover_color(gluten::theme::layer02, darkMode);
        gluten::theme::layerHover03                 = create_hover_color(gluten::theme::layer03, darkMode);

        gluten::theme::layerActive01                = create_active_color(gluten::theme::layer01, darkMode);
        gluten::theme::layerActive02                = create_active_color(gluten::theme::layer02, darkMode);
        gluten::theme::layerActive03                = create_active_color(gluten::theme::layer03, darkMode);

        gluten::theme::backgroundHover              = create_hover_color(gluten::theme::background, darkMode);
        gluten::theme::backgroundActive             = create_active_color(gluten::theme::background, darkMode);

        gluten::theme::field01                      = create_field_color(gluten::theme::layer01, darkMode);
        gluten::theme::field02                      = create_field_color(gluten::theme::layer02, darkMode);
        gluten::theme::field03                      = create_field_color(gluten::theme::layer03, darkMode);

        gluten::theme::fieldHover01                 = create_hover_color(gluten::theme::field01, darkMode);
        gluten::theme::fieldHover02                 = create_hover_color(gluten::theme::field02, darkMode);
        gluten::theme::fieldHover03                 = create_hover_color(gluten::theme::field03, darkMode);

        gluten::theme::layerAccent01                = create_layer_accent(gluten::theme::layer01, darkMode);
        gluten::theme::layerAccent02                = create_layer_accent(gluten::theme::layer02, darkMode);
        gluten::theme::layerAccent03                = create_layer_accent(gluten::theme::layer03, darkMode);

        gluten::theme::layerAccentHover01           = create_hover_color(gluten::theme::layerAccent01, darkMode);
        gluten::theme::layerAccentHover02           = create_hover_color(gluten::theme::layerAccent02, darkMode);
        gluten::theme::layerAccentHover03           = create_hover_color(gluten::theme::layerAccent03, darkMode);

        gluten::theme::layerAccentActive01          = create_active_color(gluten::theme::layerAccent01, darkMode);
        gluten::theme::layerAccentActive02          = create_active_color(gluten::theme::layerAccent02, darkMode);
        gluten::theme::layerAccentActive03          = create_active_color(gluten::theme::layerAccent03, darkMode);

        gluten::theme::borderSubtle00               = create_next_layer(gluten::theme::background, darkMode);
        gluten::theme::borderSubtle01               = create_next_layer(gluten::theme::layer01, darkMode);
        gluten::theme::borderSubtle02               = create_next_layer(gluten::theme::layer02, darkMode);
        gluten::theme::borderSubtle03               = create_next_layer(gluten::theme::layer03, darkMode);

        gluten::theme::borderStrong01               = color_with_added_value(gluten::theme::borderSubtle01, 0.10f * (darkMode ? 1.0f : -1.0f));
        gluten::theme::borderStrong02               = color_with_added_value(gluten::theme::borderSubtle02, 0.10f * (darkMode ? 1.0f : -1.0f));
        gluten::theme::borderStrong03               = color_with_added_value(gluten::theme::borderSubtle03, 0.10f * (darkMode ? 1.0f : -1.0f));

        gluten::theme::borderInteractive            = color_with_added_value(darkMode ? base00 : backgroundColor, 0.30f * (darkMode ? 1.0f : -1.0f));

        const ImVec4 textBase                       = darkMode ? base00 : backgroundColor;
        gluten::theme::textPrimary                  = color_with_multiplied_saturation(color_with_added_value(textBase, 0.75f * (darkMode ? 1.0f : -1.0f)), 0.5f);
        gluten::theme::textSecondary                = color_with_multiplied_value(gluten::theme::textPrimary, darkMode ? 0.8f : 1.3f);
        gluten::theme::textDisabled                 = color_with_multiplied_value(gluten::theme::textPrimary, darkMode ? 0.5f : 2.2f);
        gluten::theme::textPlaceholder              = color_with_multiplied_value(gluten::theme::textPrimary, darkMode ? 0.4f : 2.8f);
        gluten::theme::textHelper                   = color_with_multiplied_value(gluten::theme::textPrimary, darkMode ? 0.5f : 2.2f);
        
        gluten::theme::textOnColor                  = darkMode ? gluten::theme::white : gluten::theme::white;
        gluten::theme::textOnColorDisabled          = adjust_alpha(gluten::theme::textOnColor, 0.25f);
        gluten::theme::textInverse                  = darkMode ? gluten::theme::black : gluten::theme::white;

        gluten::theme::iconPrimary                  = gluten::theme::textPrimary;
        gluten::theme::iconSecondary                = gluten::theme::textSecondary;
        gluten::theme::iconDisabled                 = gluten::theme::textDisabled;
        gluten::theme::iconOnColor                  = gluten::theme::textOnColor;
        gluten::theme::iconOnColorDisabled          = gluten::theme::textOnColorDisabled;
        gluten::theme::iconInverse                  = gluten::theme::textInverse;
        gluten::theme::iconInteractive              = gluten::theme::interactive;

        gluten::theme::supportError                 = red;
        gluten::theme::supportSuccess               = green;
        gluten::theme::supportWarning               = yellow;
        gluten::theme::supportInfo                  = color_with_multiplied_value(color_with_multiplied_saturation(blue, 0.5f), 0.75f);

        gluten::theme::interactive                  = blue;
        gluten::theme::interactiveHover             = create_hover_color(blue, darkMode);
        gluten::theme::interactiveActive            = create_active_color(blue, darkMode);

        gluten::theme::interactiveSecondary         = gluten::theme::layer01;
        gluten::theme::interactiveSecondaryHover    = gluten::theme::layerHover01;

        gluten::theme::shadow                       = ImVec4(borderStrong01.x, borderStrong01.y, borderStrong01.z, 0.5f);
    }
}  // namespace gluten::theme::things