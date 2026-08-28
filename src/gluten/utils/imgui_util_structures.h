#pragma once

#include "gluten/app/app.h"
#include "gluten/theme/theme.h"
#include "imgui.h"
#include "imgui_internal.h"

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

    // Push a semantic text style (size + font family) for the duration of
    // the scope. Prefer this over ad-hoc PushFont(font, px) at call sites --
    // it keeps size decisions in one place (gluten::theme::textSize*) and
    // means a future scale tweak lands everywhere at once.
    //
    // The family override is escape-hatch only; leaving it default lets
    // theme::text_font_for pick the right face for the style.
    struct scoped_text_style
    {
        scoped_text_style(const scoped_text_style&)           = delete;
        scoped_text_style operator=(const scoped_text_style&) = delete;

        explicit scoped_text_style(text_style style)
        {
            ImGui::PushFont(gluten::app::get()->get_font(gluten::theme::text_font_for(style)), gluten::theme::text_size_for(style));
        }

        scoped_text_style(text_style style, fonts fontOverride)
        {
            ImGui::PushFont(gluten::app::get()->get_font(fontOverride), gluten::theme::text_size_for(style));
        }

        ~scoped_text_style() { ImGui::PopFont(); }
    };

    struct scoped_button_style
    {
        scoped_button_style(const scoped_button_style&)           = delete;
        scoped_button_style operator=(const scoped_button_style&) = delete;

        explicit scoped_button_style(button_style style)
        {
            const auto colors = gluten::theme::button_colors_for(style);
            ImGui::PushStyleColor(ImGuiCol_Button, colors.button);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors.hovered);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, colors.active);
            ImGui::PushStyleColor(ImGuiCol_Text, colors.text);
        }

        ~scoped_button_style() { ImGui::PopStyleColor(4); }
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

    struct scoped_context_menu
    {
        scoped_context_menu(const scoped_context_menu&)           = delete;
        scoped_context_menu operator=(const scoped_context_menu&) = delete;

        explicit scoped_context_menu(const char* id, ImGuiPopupFlags flags = ImGuiPopupFlags_MouseButtonRight)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(gluten::theme::space04, gluten::theme::space08));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, gluten::theme::space04));
            ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.0f, 0.5f));

            ImGui::PushStyleColor(ImGuiCol_PopupBg, gluten::theme::layer03);
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, gluten::theme::layerHover03);
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, gluten::theme::layerActive03);

            ImGui::SetNextWindowSizeConstraints(ImVec2(200.0f, 0.0f), ImVec2(FLT_MAX, FLT_MAX));

            m_open = ImGui::BeginPopupContextItem(id, flags);

            if (m_open)
            {
                ImGui::PushFont(
                    gluten::app::get()->get_font(gluten::theme::text_font_for(gluten::text_style::body)),
                    gluten::theme::text_size_for(gluten::text_style::body));
            }
        }

        ~scoped_context_menu()
        {
            if (m_open)
            {
                ImGui::PopFont();
                ImGui::EndPopup();
            }
            ImGui::PopStyleColor(3);
            ImGui::PopStyleVar(3);
        }

        explicit operator bool() const { return m_open; }

    private:
        bool m_open = false;
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

    template <typename T>
    concept scoped_storage_type = std::is_same_v<T, float> || std::is_same_v<T, int> || std::is_same_v<T, bool>;

    template <scoped_storage_type T>
    struct scoped_state_storage
    {
        scoped_state_storage() = delete;

        scoped_state_storage(const char* inId, T defaultValue = T{}) : id(ImGui::GetID(inId))
        {
            if constexpr (std::is_floating_point_v<T>)
            {
                value = ImGui::GetStateStorage()->GetFloat(id, defaultValue);
            }
            else if constexpr (std::is_same_v<T, int>)
            {
                value = ImGui::GetStateStorage()->GetInt(id, defaultValue);
            }
            else if constexpr (std::is_same_v<T, bool>)
            {
                value = ImGui::GetStateStorage()->GetBool(id, defaultValue);
            }
        }

        ~scoped_state_storage()
        {
            if constexpr (std::is_floating_point_v<T>)
            {
                ImGui::GetStateStorage()->SetFloat(id, value);
            }
            else if constexpr (std::is_same_v<T, int>)
            {
                ImGui::GetStateStorage()->SetInt(id, value);
            }
            else if constexpr (std::is_same_v<T, bool>)
            {
                ImGui::GetStateStorage()->SetBool(id, value);
            }
        }

        auto operator&() noexcept -> bool* { return &value; }
        auto operator&() const noexcept -> const bool* { return &value; }

        operator T() const noexcept { return value; }
        auto operator=(T newValue) -> scoped_state_storage<T>&
        {
            value = newValue;
            return *this;
        }

        ImGuiID id{};
        T value{};
    };
}  // namespace gluten::imgui