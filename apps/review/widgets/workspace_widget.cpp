#include "workspace_widget.h"

#include "IconsLucide.h"
#include "gluten/elements/layouts/layout.h"
#include "gluten/elements/icon_button.h"
#include "gluten/theme/carbon_theme_g100.h"
#include "gluten/utils/imgui_util_structures.h"

#include "managers/workspace_manager.h"

namespace
{
    constexpr float leftToobarWidth             = 100.0f;
    constexpr float leftToolbarButtonHeight     = leftToobarWidth;
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
    const ImVec2 leftToolbarSize(leftToobarWidth, windowSize.y);
    ImVec2 nextCursorPos = ImGui::GetCursorPos();
    nextCursorPos.x += leftToobarWidth;

    gluten::imgui::scoped_color toolbarBackgroundColor(ImGuiCol_ChildBg, gluten::theme::carbon_g100::field02);

    if (ImGui::BeginChild("LeftToolbar", leftToolbarSize))
    {
        gluten::layout buttonsLayout(gluten::layout::layout_type::top_to_bottom, gluten::element::anchor_preset::stretch_full);
        buttonsLayout.render_window();

        gluten::icon_button reviewsButton("##ReviewsButton", ICON_LC_CHART_NO_AXES_GANTT, gluten::fonts::regular_lucide_icons);
        gluten::icon_button usersButton("##UsersButton", ICON_LC_USERS, gluten::fonts::regular_lucide_icons);

        reviewsButton.set_element_min_size(ImVec2(leftToolbarButtonHeight, leftToolbarButtonHeight));
        usersButton.set_element_min_size(ImVec2(leftToolbarButtonHeight, leftToolbarButtonHeight));

        reviewsButton.set_element_scale(3.0f);
        usersButton.set_element_scale(3.0f);

        reviewsButton.set_element_hover_color(gluten::theme::carbon_g100::fieldHover02);
        usersButton.set_element_hover_color(gluten::theme::carbon_g100::fieldHover02);

        reviewsButton.set_element_active_color(gluten::theme::carbon_g100::field01);
        usersButton.set_element_active_color(gluten::theme::carbon_g100::field01);

        reviewsButton.set_element_active(m_activeView == reviews_view);
        usersButton.set_element_active(m_activeView == users_view);

        if (buttonsLayout.render_layout_element_pixels_vertical(&reviewsButton, leftToolbarButtonHeight))
        {
            m_activeView = reviews_view;
        }

        if (buttonsLayout.render_layout_element_pixels_vertical(&usersButton, leftToolbarButtonHeight))
        {
            m_activeView = users_view;
        }
    }
    ImGui::EndChild();

    ImGui::SetCursorPos(nextCursorPos);

    switch (m_activeView)
    {
        case workspace_widget::reviews_view:
            ImGui::TextUnformatted("Reviews");
            break;
        case workspace_widget::users_view:
            ImGui::TextUnformatted("Users");
            break;
        default:
            break;
    }
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