#pragma once

#include "gluten/theme/theme.h"

namespace gluten::theme::things
{
    static ImVec4 base00 = ImColor::HSV(220 / 360.f, 13 / 100.0f, 12 / 100.f);
    //constexpr ImVec4 base00 = hex_to_imgui_imvec4(0x202225);
    //constexpr ImVec4 base00 = hex_to_imgui_imvec4(0x24262a);

    static ImVec4 blue   = hex_to_imgui_imvec4(0x2e80f2);
    static ImVec4 pink   = hex_to_imgui_imvec4(0xff82b2);
    static ImVec4 green  = hex_to_imgui_imvec4(0x3eb4bf);
    static ImVec4 yellow = hex_to_imgui_imvec4(0xe5b567);
    static ImVec4 orange = hex_to_imgui_imvec4(0xe87d3e);
    static ImVec4 red    = hex_to_imgui_imvec4(0xe83e3e);
    static ImVec4 purple = hex_to_imgui_imvec4(0x9e86c8);

    auto create_hover_color(const ImVec4& color) -> ImVec4
    {
        return color_with_added_value(color, 0.08f);
    }

    auto create_active_color(const ImVec4& color) -> ImVec4
    {
        return color_with_added_value(color, 0.1f);
    }

    auto create_next_layer(const ImVec4& color) -> ImVec4
    {
        return color_with_multiplied_saturation(color_with_added_value(color, 0.05f), 0.88f);
    }

    auto create_field_color(const ImVec4& color) -> ImVec4
    {
        return color_with_multiplied_saturation(color, 1.15f);
    }

    auto create_layer_accent(const ImVec4& color) -> ImVec4
    {
        return color_with_multiplied_saturation(color_with_added_value(color, 0.06f), 1.10f);
    }

    inline void apply_colours()
    {
        gluten::theme::background   = base00;
        gluten::theme::layer01      = create_next_layer(gluten::theme::background);
        gluten::theme::layer02      = create_next_layer(gluten::theme::layer01);
        gluten::theme::layer03      = create_next_layer(gluten::theme::layer02);

        gluten::theme::layerHover01 = create_hover_color(gluten::theme::layer01);
        gluten::theme::layerHover02 = create_hover_color(gluten::theme::layer02);
        gluten::theme::layerHover03 = create_hover_color(gluten::theme::layer03);

        gluten::theme::layerActive01 = create_active_color(gluten::theme::layer01);
        gluten::theme::layerActive02 = create_active_color(gluten::theme::layer02);
        gluten::theme::layerActive03 = create_active_color(gluten::theme::layer03);

        gluten::theme::backgroundHover  = create_hover_color(gluten::theme::background);
        gluten::theme::backgroundActive = create_active_color(gluten::theme::background);

        gluten::theme::field01 = create_field_color(gluten::theme::layer01);
        gluten::theme::field02 = create_field_color(gluten::theme::layer02);
        gluten::theme::field03 = create_field_color(gluten::theme::layer03);

        gluten::theme::fieldHover01 = create_hover_color(gluten::theme::field01);
        gluten::theme::fieldHover02 = create_hover_color(gluten::theme::field02);
        gluten::theme::fieldHover03 = create_hover_color(gluten::theme::field03);

        gluten::theme::layerAccent01 = create_layer_accent(gluten::theme::layer01);
        gluten::theme::layerAccent02 = create_layer_accent(gluten::theme::layer02);
        gluten::theme::layerAccent03 = create_layer_accent(gluten::theme::layer03);

        gluten::theme::layerAccentHover01 = create_hover_color(gluten::theme::layerAccent01);
        gluten::theme::layerAccentHover02 = create_hover_color(gluten::theme::layerAccent02);
        gluten::theme::layerAccentHover03 = create_hover_color(gluten::theme::layerAccent03);
        
        gluten::theme::layerAccentActive01 = create_active_color(gluten::theme::layerAccent01);
        gluten::theme::layerAccentActive02 = create_active_color(gluten::theme::layerAccent02);
        gluten::theme::layerAccentActive03 = create_active_color(gluten::theme::layerAccent03);

        gluten::theme::borderSubtle00 = create_next_layer(gluten::theme::background);
        gluten::theme::borderSubtle01 = create_next_layer(gluten::theme::layer01);
        gluten::theme::borderSubtle02 = create_next_layer(gluten::theme::layer02);
        gluten::theme::borderSubtle03 = create_next_layer(gluten::theme::layer03);

        gluten::theme::borderStrong01 = color_with_added_value(gluten::theme::borderSubtle01, 0.10f);
        gluten::theme::borderStrong02 = color_with_added_value(gluten::theme::borderSubtle02, 0.10f);
        gluten::theme::borderStrong03 = color_with_added_value(gluten::theme::borderSubtle03, 0.10f);

        gluten::theme::borderInteractive = color_with_added_value(base00, 0.30f);

        gluten::theme::textPrimary = white;
        gluten::theme::textSecondary = color_with_multiplied_value(gluten::theme::textPrimary, 0.8f);
        gluten::theme::textDisabled  = color_with_multiplied_value(gluten::theme::textPrimary, 0.5f);

        gluten::theme::supportError = red;
        gluten::theme::supportSuccess = green;
        gluten::theme::supportWarning = yellow;

        gluten::theme::interactive = gluten::theme::layer03;
        gluten::theme::interactiveHover  = gluten::theme::layerHover03;
        gluten::theme::interactiveActive = gluten::theme::layerActive03;
    }
}