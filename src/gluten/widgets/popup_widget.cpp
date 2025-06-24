#include "popup_widget.h"

#include "gluten/theme/carbon_theme_g100.h"
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

    gluten::imgui::scoped_color inputTextBackground(ImGuiCol_FrameBg, gluten::theme::carbon_g100::layer01);
    gluten::imgui::scoped_style popupPadding(ImGuiStyleVar_WindowPadding, ImVec2(gluten::theme::carbon_g100::paddingVec));

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(800, 250), ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal(get_widget_name().data(), &m_visible, ImGuiWindowFlags_AlwaysAutoResize))
    {
        render_popup();

        ImGui::EndPopup();
    }
}