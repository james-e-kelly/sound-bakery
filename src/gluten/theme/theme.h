#pragma once

#include "pch.h"
#include "imgui.h"

namespace gluten::theme
{
    constexpr auto invalidPrefab   = IM_COL32(222, 43, 43, 255);

    static inline float Convert_sRGB_FromLinear(float theLinearValue);
    static inline float Convert_sRGB_ToLinear(float thesRGBValue);
    ImVec4 ConvertFromSRGB(ImVec4 colour);
    ImVec4 ConvertToSRGB(ImVec4 colour);

    inline float Convert_sRGB_FromLinear(float theLinearValue)
    {
        return theLinearValue <= 0.0031308f ? theLinearValue * 12.92f
                                            : std::pow<float>(theLinearValue, 1.0f / 2.2f) * 1.055f - 0.055f;
    }

    inline float Convert_sRGB_ToLinear(float thesRGBValue)
    {
        return thesRGBValue <= 0.04045f ? thesRGBValue / 12.92f
                                        : std::pow<float>((thesRGBValue + 0.055f) / 1.055f, 2.2f);
    }

    inline ImVec4 ConvertFromSRGB(ImVec4 colour)
    {
        return ImVec4(Convert_sRGB_FromLinear(colour.x), Convert_sRGB_FromLinear(colour.y),
                        Convert_sRGB_FromLinear(colour.z), colour.w);
    }

    inline ImVec4 ConvertToSRGB(ImVec4 colour)
    {
        return ImVec4(std::pow(colour.x, 2.2f), std::pow<float>(colour.y, 2.2f), std::pow<float>(colour.z, 2.2f),
                        colour.w);
    }

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
}