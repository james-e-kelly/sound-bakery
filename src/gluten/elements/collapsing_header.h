#pragma once

#include "gluten/pch.h"

#include "gluten/elements/element.h"

namespace gluten
{
    class collapsing_header : public gluten::element
    {
    public:
        collapsing_header(const std::string& label, bool defaultOpen = true, const std::string& detail = {})
            : element(anchor_preset::stretch_full), m_label(label), m_defaultOpen(defaultOpen), m_detail(detail)
        {
            set_element_background_color(ImGui::GetStyleColorVec4(ImGuiCol_Header));
            set_element_hover_color(ImGui::GetStyleColorVec4(ImGuiCol_HeaderHovered));
            set_element_rounding(m_elementRounding);
            set_element_inner_padding(gluten::theme::insetCompact);
        }

        auto set_open(bool open) -> void;

        auto get_open() const -> bool;

    protected:
        auto render_element(const element_render_info& renderInfo) -> bool override;
        auto get_element_content_size(const ImVec2& parentSize) -> ImVec2 const override
        {
            const ImVec2 textSize = ImGui::CalcTextSize(m_label.c_str());
            return ImVec2(0.0f, textSize.y + (ImGui::GetStyle().FramePadding.y * 2.0f));
        }

        std::string m_label;
        std::string m_detail;
        bool m_defaultOpen = true;
    };
}  // namespace gluten