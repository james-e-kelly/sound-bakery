#pragma once

#include "imgui.h"
#include "gluten/pch.h"

namespace gluten::theme
{
    constexpr auto invalidPrefab = IM_COL32(222, 43, 43, 255);

    inline ImU32 ColorWithValue(const ImColor& color, float value)
    {
        const ImVec4& colRow = color.Value;
        float hue, sat, val;
        ImGui::ColorConvertRGBtoHSV(colRow.x, colRow.y, colRow.z, hue, sat, val);
        return ImColor::HSV(hue, sat, std::min(value, 1.0f));
    }

    inline ImU32 ColorWithSaturation(const ImColor& color, float saturation)
    {
        const ImVec4& colRow = color.Value;
        float hue, sat, val;
        ImGui::ColorConvertRGBtoHSV(colRow.x, colRow.y, colRow.z, hue, sat, val);
        return ImColor::HSV(hue, std::min(saturation, 1.0f), val);
    }

    inline ImU32 ColorWithHue(const ImColor& color, float hue)
    {
        const ImVec4& colRow = color.Value;
        float h, s, v;
        ImGui::ColorConvertRGBtoHSV(colRow.x, colRow.y, colRow.z, h, s, v);
        return ImColor::HSV(std::min(hue, 1.0f), s, v);
    }

    inline ImU32 ColorWithMultipliedValue(const ImColor& color, float multiplier)
    {
        const ImVec4& colRow = color.Value;
        float hue, sat, val;
        ImGui::ColorConvertRGBtoHSV(colRow.x, colRow.y, colRow.z, hue, sat, val);
        return ImColor::HSV(hue, sat, std::min(val * multiplier, 1.0f));
    }

    inline ImU32 ColorWithMultipliedSaturation(const ImColor& color, float multiplier)
    {
        const ImVec4& colRow = color.Value;
        float hue, sat, val;
        ImGui::ColorConvertRGBtoHSV(colRow.x, colRow.y, colRow.z, hue, sat, val);
        return ImColor::HSV(hue, std::min(sat * multiplier, 1.0f), val);
    }

    inline ImU32 ColorWithMultipliedHue(const ImColor& color, float multiplier)
    {
        const ImVec4& colRow = color.Value;
        float hue, sat, val;
        ImGui::ColorConvertRGBtoHSV(colRow.x, colRow.y, colRow.z, hue, sat, val);
        return ImColor::HSV(std::min(hue * multiplier, 1.0f), sat, val);
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
}  // namespace gluten::theme
