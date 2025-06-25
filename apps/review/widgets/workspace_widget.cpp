#include "workspace_widget.h"

#include "IconsLucide.h"
#include "gluten/elements/layouts/layout.h"
#include "gluten/elements/button.h"
#include "gluten/theme/carbon_theme_g100.h"
#include "gluten/utils/imgui_util_structures.h"

#include "managers/workspace_manager.h"

namespace
{
    constexpr float leftToolbarButtonHeight = 50.f;
    constexpr float leftToolbarHalfButtonHeight = leftToolbarButtonHeight / 2.0f;
}

auto workspace_widget::start_implementation() -> void
{
    set_window_flags(ImGuiWindowFlags_NoDecoration);

    ImGuiWindowClass windowClass;
    windowClass.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoTabBar;

    set_window_class(windowClass);
}

auto workspace_widget::render_window_implementation() -> void
{
    const ImVec2 windowSize = ImGui::GetCurrentWindow()->WorkRect.GetSize();
    const ImVec2 leftToolbarSize(100.0f, windowSize.y);

    gluten::imgui::scoped_color toolbarBackgroundColor(ImGuiCol_ChildBg, gluten::theme::carbon_g100::field02);

    if (ImGui::BeginChild("LeftToolbar", leftToolbarSize))
    {
        gluten::imgui::scoped_font font(get_app()->get_font(gluten::fonts::regular_lucide_icons));
        gluten::imgui::scoped_color buttonColor(ImGuiCol_Button, gluten::theme::carbon_g100::field02);
        gluten::imgui::scoped_color buttonHoverColor(ImGuiCol_ButtonHovered, gluten::theme::carbon_g100::fieldHover02);
        gluten::imgui::scoped_color buttonActiveColor(ImGuiCol_ButtonActive, gluten::theme::carbon_g100::field02);

        gluten::layout buttonsLayout(gluten::layout::layout_type::top_to_bottom, gluten::element::anchor_preset::stretch_full);
        buttonsLayout.set_element_window_padding();
        buttonsLayout.set_layout_spacing(leftToolbarHalfButtonHeight);
        buttonsLayout.get_element_anchor().minOffset = ImVec2(0.0f, leftToolbarHalfButtonHeight);

        buttonsLayout.render_window();

        const float buttonBoxSize = std::min(buttonsLayout.get_element_rect().GetSize().x, leftToolbarButtonHeight) * 1.5f;
        const ImVec2 buttonBox    = ImVec2(buttonBoxSize, buttonBoxSize);

        gluten::button reviewsButton(ICON_LC_SEARCH, false, gluten::element::anchor_preset::center_middle);
        gluten::button usersButton(ICON_LC_USERS, false, gluten::element::anchor_preset::center_middle);

        /*reviewsButton.set_element_min_size(buttonBox);
        usersButton.set_element_min_size(buttonBox);*/

        reviewsButton.set_element_alignment(ImVec2(0.5f, 0.5f));
        usersButton.set_element_alignment(ImVec2(0.5f, 0.5f));

        reviewsButton.set_element_scale(2.0f);
        usersButton.set_element_scale(2.0f);

        buttonsLayout.render_layout_element_pixels_vertical(&reviewsButton, leftToolbarButtonHeight);
        buttonsLayout.render_layout_element_pixels_vertical(&usersButton, leftToolbarButtonHeight);
    }
    ImGui::EndChild();

	ImGui::TextUnformatted("Workspace");
}

auto workspace_widget::render_menu_implementation() -> void
{
    if (ImGui::BeginMenu(s_fileMenuName))
    {
        if (ImGui::MenuItem("Close Workspace"))
        {
            get_app()->get_manager_by_class<workspace_manager>()->close_workspace();
        }
        ImGui::EndMenu();
    }
}