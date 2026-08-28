#include "popup_widget.h"

#include "gluten/elements/loading_spinner.h"
#include "gluten/theme/theme.h"
#include "gluten/utils/imgui_util_structures.h"

auto gluten::popup_widget::open_popup() -> void
{
    if (!ImGui::IsPopupOpen(get_widget_name().data()))
    {
        ImGui::OpenPopup(get_widget_name().data());
        set_visibile(true);
    }
}

auto gluten::popup_widget::close_popup() -> void
{
    if (ImGui::IsPopupOpen(get_widget_name().data()))
    {
        ImGui::CloseCurrentPopup();
    }
    set_visibile(false);
}

auto gluten::popup_widget::start_implementation() -> void
{
    set_visible_in_toolbar(false, false);
}

auto gluten::popup_widget::render_implementation() -> void
{
    open_popup();

    gluten::imgui::scoped_color windowBackground(ImGuiCol_WindowBg, gluten::theme::background);
    gluten::imgui::scoped_color inputTextBackground(ImGuiCol_FrameBg, gluten::theme::layer01);
    gluten::imgui::scoped_style popupPadding(ImGuiStyleVar_WindowPadding, ImVec2(gluten::theme::insetFrame));

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(gluten::theme::insetWindow * 10.0f, ImGuiCond_Appearing);

    bool popupOpen = false;

    switch (m_style)
    {
        case popup_style::modal:
        {
            if (ImGui::BeginPopupModal(get_widget_name().data(), m_closable ? &m_visible : nullptr))
            {
                popupOpen = true;
                render_popup();

                ImGui::EndPopup();
            }
            break;
        }
        case popup_style::normal:
        {
            if (ImGui::BeginPopup(get_widget_name().data()))
            {
                popupOpen = true;
                render_popup();

                ImGui::EndPopup();
            }
            break;
        }
    }

    if (!popupOpen)
    {
        set_visibile(false);
    }
}

auto gluten::confirmation_popup::render_popup() -> void
{
    ImGui::TextUnformatted("Are you sure?");

    if (ImGui::Button("Confirm"))
    {
        if (m_onConfirm)
        {
            m_onConfirm();
        }
        close_popup();
    }

    ImGui::SameLine();

    if (ImGui::Button("Cancel"))
    {
        close_popup();
    }
}

auto gluten::loading_popup::start_implementation() -> void
{
    set_closable(false);
}

auto gluten::loading_popup::render_popup() -> void
{
    ImGui::ProgressBar(-1.0f * (float)ImGui::GetTime(), ImVec2(s_progressBarWidth, 0.0f), "Loading...");
}