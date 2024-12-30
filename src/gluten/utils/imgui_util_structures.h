#pragma once

#include "imgui.h"

namespace gluten::imgui
{
    struct scoped_style
    {
        scoped_style(const scoped_style&)           = delete;
        scoped_style operator=(const scoped_style&) = delete;
        template <typename T>
        scoped_style(ImGuiStyleVar styleVar, T value)
        {
            ImGui::PushStyleVar(styleVar, value);
        }
        ~scoped_style() { ImGui::PopStyleVar(); }
    };

    struct scoped_color
    {
        scoped_color(const scoped_color&)           = delete;
        scoped_color operator=(const scoped_color&) = delete;
        template <typename T>
        scoped_color(ImGuiCol ColorId, T Color)
        {
            ImGui::PushStyleColor(ColorId, Color);
        }
        ~scoped_color() { ImGui::PopStyleColor(); }
    };

    struct scoped_font
    {
        scoped_font(const scoped_font&)           = delete;
        scoped_font operator=(const scoped_font&) = delete;
        scoped_font(ImFont* font) { ImGui::PushFont(font); }
        ~scoped_font() { ImGui::PopFont(); }
    };

    struct scoped_id
    {
        scoped_id(const scoped_id&)           = delete;
        scoped_id operator=(const scoped_id&) = delete;
        template <typename T>
        scoped_id(T id)
        {
            ImGui::PushID(id);
        }
        ~scoped_id() { ImGui::PopID(); }
    };

    struct scoped_color_stack
    {
        scoped_color_stack(const scoped_color_stack&)           = delete;
        scoped_color_stack operator=(const scoped_color_stack&) = delete;

        template <typename ColorType, typename... OtherColors>
        scoped_color_stack(ImGuiCol firstColorID, ColorType firstColor, OtherColors&&... otherColorPairs)
            : m_count((sizeof...(otherColorPairs) / 2) + 1)
        {
            static_assert(
                (sizeof...(otherColorPairs) & 1u) == 0,
                "scoped_color_stack constructor expects a list of pairs of Color IDs and Colors as its arguments");

            PushColor(firstColorID, firstColor, std::forward<OtherColors>(otherColorPairs)...);
        }

        ~scoped_color_stack() { ImGui::PopStyleColor(m_count); }

    private:
        int m_count;

        template <typename ColorType, typename... OtherColors>
        void PushColor(ImGuiCol ColorID, ColorType Color, OtherColors&&... otherColorPairs)
        {
            if constexpr (sizeof...(otherColorPairs) == 0)
            {
                ImGui::PushStyleColor(ColorID, Color);
            }
            else
            {
                ImGui::PushStyleColor(ColorID, Color);
                PushColor(std::forward<OtherColors>(otherColorPairs)...);
            }
        }
    };

    struct scoped_style_stack
    {
        scoped_style_stack(const scoped_style_stack&)           = delete;
        scoped_style_stack operator=(const scoped_style_stack&) = delete;

        template <typename ValueType, typename... OtherStylePairs>
        scoped_style_stack(ImGuiStyleVar firstStyleVar, ValueType firstValue, OtherStylePairs&&... otherStylePairs)
            : m_count((sizeof...(otherStylePairs) / 2) + 1)
        {
            static_assert(
                (sizeof...(otherStylePairs) & 1u) == 0,
                "scoped_style_stack constructor expects a list of pairs of Color IDs and Colors as its arguments");

            PushStyle(firstStyleVar, firstValue, std::forward<OtherStylePairs>(otherStylePairs)...);
        }

        ~scoped_style_stack() { ImGui::PopStyleVar(m_count); }

    private:
        int m_count;

        template <typename ValueType, typename... OtherStylePairs>
        void PushStyle(ImGuiStyleVar styleVar, ValueType value, OtherStylePairs&&... otherStylePairs)
        {
            if constexpr (sizeof...(otherStylePairs) == 0)
            {
                ImGui::PushStyleVar(styleVar, value);
            }
            else
            {
                ImGui::PushStyleVar(styleVar, value);
                PushStyle(std::forward<OtherStylePairs>(otherStylePairs)...);
            }
        }
    };

    struct scoped_item_flags
    {
        scoped_item_flags(const scoped_item_flags&)           = delete;
        scoped_item_flags operator=(const scoped_item_flags&) = delete;
        scoped_item_flags(const ImGuiItemFlags flags, const bool enable = true) { ImGui::PushItemFlag(flags, enable); }
        ~scoped_item_flags() { ImGui::PopItemFlag(); }
    };

    struct scoped_window
    {
        scoped_window(const scoped_item_flags&)           = delete;
        scoped_window operator=(const scoped_item_flags&) = delete;
        scoped_window(const char* name, bool* open = nullptr, ImGuiWindowFlags flags = 0)
        {
            ImGui::Begin(name, open, flags);
        }
        ~scoped_window() { ImGui::End(); }
    };

    struct resize_border_def
    {
        ImVec2 InnerDir;
        ImVec2 SegmentN1, SegmentN2;
        float OuterAngle;
    };

    const inline resize_border_def resize_border[4] = {
        {ImVec2(+1, 0), ImVec2(0, 1), ImVec2(0, 0), IM_PI * 1.00f},  // Left
        {ImVec2(-1, 0), ImVec2(1, 0), ImVec2(1, 1), IM_PI * 0.00f},  // Right
        {ImVec2(0, +1), ImVec2(0, 0), ImVec2(1, 0), IM_PI * 1.50f},  // Up
        {ImVec2(0, -1), ImVec2(1, 1), ImVec2(0, 1), IM_PI * 0.50f}   // Down
    };
}  // namespace gluten::imgui