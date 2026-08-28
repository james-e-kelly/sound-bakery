#include "combo.h"

#include "gluten/app/app.h"
#include "gluten/theme/theme.h"
#include "gluten/utils/imgui_util_structures.h"
#include "IconsLucide.h"
#include "imgui_internal.h"

gluten::combo::combo(const char* id, std::span<const char* const> options, int* selectedIndex)
    : element(anchor_preset::left_top), m_id(id), m_options(options), m_selectedIndex(selectedIndex)
{
}

auto gluten::combo::set_button_style(button_style style) -> combo&
{
    m_buttonStyle = style;
    return *this;
}

bool gluten::combo::render_element(const element_render_info& renderInfo)
{
    bool changed = false;

    const ImVec2 boxSize = renderInfo.elementBox.GetSize();
    if (boxSize.x < 0.1f || boxSize.y < 0.1f)
    {
        return false;
    }

    const int currentIndex  = (m_selectedIndex && *m_selectedIndex >= 0 && *m_selectedIndex < static_cast<int>(m_options.size())) ? *m_selectedIndex : 0;
    const char* previewText = m_options.empty() ? "" : m_options[currentIndex];

    std::optional<gluten::imgui::scoped_button_style> styleScope;
    if (m_buttonStyle.has_value())
    {
        styleScope.emplace(m_buttonStyle.value());
    }

    const std::string popupLabel = fmt::format("##combo_popup_{}", m_id);

    if (ImGui::InvisibleButton(m_id, boxSize))
    {
        ImGui::OpenPopup(popupLabel.c_str());
    }

    const bool hovered = ImGui::IsItemHovered();
    const bool active  = ImGui::IsItemActive();

    const auto colors = m_buttonStyle.has_value() ? gluten::theme::button_colors_for(m_buttonStyle.value())
                                                  : gluten::theme::button_colors_for(gluten::button_style::primary);

    const ImU32 bgColor = active ? ImGui::ColorConvertFloat4ToU32(colors.active)
                        : hovered ? ImGui::ColorConvertFloat4ToU32(colors.hovered)
                                  : ImGui::ColorConvertFloat4ToU32(colors.button);

    ImDrawList* drawList   = ImGui::GetWindowDrawList();
    const float rounding   = ImGui::GetStyle().FrameRounding;
    const ImVec2 padding   = ImGui::GetStyle().FramePadding;

    drawList->AddRectFilled(renderInfo.elementBox.Min, renderInfo.elementBox.Max, bgColor, rounding);

    const ImU32 textColor     = ImGui::ColorConvertFloat4ToU32(colors.text);
    const float textY         = renderInfo.elementBox.Min.y + (boxSize.y - ImGui::GetTextLineHeight()) * 0.5f;
    const ImVec2 labelPos(renderInfo.elementBox.Min.x + padding.x, textY);
    drawList->AddText(labelPos, textColor, previewText);

    const ImVec2 chevronSize = ImGui::CalcTextSize(ICON_LC_CHEVRON_DOWN);
    const ImVec2 chevronPos(renderInfo.elementBox.Max.x - padding.x - chevronSize.x, textY);
    drawList->AddText(chevronPos, textColor, ICON_LC_CHEVRON_DOWN);

    const float fontScale = ImGui::GetCurrentWindow()->FontWindowScale;

    const ImVec2 popupPos(renderInfo.elementBox.Min.x, renderInfo.elementBox.Max.y + gluten::theme::space04);
    ImGui::SetNextWindowPos(popupPos, ImGuiCond_Appearing);

    gluten::imgui::scoped_style popupRounding(ImGuiStyleVar_PopupRounding, gluten::theme::radiusLg);
    gluten::imgui::scoped_style popupPadding(ImGuiStyleVar_WindowPadding, gluten::theme::insetFrame);
    gluten::imgui::scoped_color popupBg(ImGuiCol_PopupBg, gluten::theme::layer02);

    if (ImGui::BeginPopup(popupLabel.c_str()))
    {
        ImGui::SetWindowFontScale(fontScale);

        for (int i = 0; i < static_cast<int>(m_options.size()); ++i)
        {
            const bool isSelected = (i == currentIndex);

            gluten::imgui::scoped_color hoveredCol(ImGuiCol_HeaderHovered, gluten::theme::layerHover02);
            gluten::imgui::scoped_color activeCol(ImGuiCol_HeaderActive, gluten::theme::layerActive02);

            if (ImGui::Selectable(m_options[i], isSelected))
            {
                if (m_selectedIndex)
                {
                    *m_selectedIndex = i;
                }
                changed = true;
            }
        }

        ImGui::EndPopup();
    }

    return changed;
}

auto gluten::combo::get_element_content_size(const ImVec2& parentSize) -> ImVec2 const
{
    float textWidth = 0.0f;
    for (const char* option : m_options)
    {
        const float w = ImGui::CalcTextSize(option, nullptr, true).x;
        if (w > textWidth)
        {
            textWidth = w;
        }
    }

    const float chevronWidth = ImGui::CalcTextSize(ICON_LC_CHEVRON_DOWN, nullptr, true).x;
    const ImVec2 padding     = ImGui::GetStyle().FramePadding;
    const float spacing      = gluten::theme::space08;

    return ImVec2(textWidth + spacing + chevronWidth + padding.x * 2.0f, ImGui::GetTextLineHeight() + padding.y * 2.0f);
}
