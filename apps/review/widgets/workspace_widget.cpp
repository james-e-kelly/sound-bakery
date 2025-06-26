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

    constexpr float itemListWidth = leftToobarWidth * 5.0f;
}

auto workspace_widget::start_implementation() -> void
{
    ImGuiWindowClass windowClass;
    windowClass.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoTabBar;

    set_window_class(windowClass);
}

auto workspace_widget::render_window_implementation() -> void
{
    gluten::imgui::scoped_style noGaps(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

    render_left_toolbar();
    
    ImGui::SameLine();

    switch (m_activeView)
    {
        case workspace_widget::reviews_view:
            render_list();
            ImGui::SameLine();
            render_content();
            break;
        case workspace_widget::users_view:
            ImGui::TextUnformatted("Users");
            break;
        default:
            break;
    }
}

void workspace_widget::render_list()
{
    gluten::imgui::scoped_color backgroundColor(ImGuiCol_ChildBg, gluten::theme::carbon_g100::field02);

    if (ImGui::BeginChild("ItemsList", ImVec2(itemListWidth, 0), ImGuiChildFlags_ResizeX))
    {
        gluten::element topToolbar(gluten::element::anchor_preset::stretch_top);
        topToolbar.get_element_anchor().maxOffset.y = leftToobarWidth;
        topToolbar.render_window();

        gluten::text titleText("Projects", ImVec2(0.5f, 0.5f), gluten::element::anchor_preset::center_middle);
        titleText
            .set_font(gluten::fonts::title)
            .set_font_size(24.0f)
            .render(topToolbar.get_element_rect());

        gluten::icon_button backButton("##BackButton", ICON_LC_ARROW_BIG_LEFT, gluten::fonts::regular_lucide_icons);
        backButton
            .set_element_anchor_preset(gluten::element::anchor_preset::left_middle)
            .set_element_min_size(ImVec2(25.0f, 25.0f))
            .set_element_scale(3.0f)
            .render(topToolbar.get_element_rect());

        ImGui::SetCursorPos(topToolbar.get_element_rect().GetBL());

        ImGui::TextUnformatted("Item List");
        ImGui::Separator();
    }
    ImGui::EndChild();
}

void workspace_widget::render_content()
{
    gluten::imgui::scoped_color backgroundColor(ImGuiCol_ChildBg, gluten::theme::carbon_g100::field03);

    if (ImGui::BeginChild("Content"))
    {
        ImGui::TextUnformatted("Content");
    }
    ImGui::EndChild();
}

void workspace_widget::render_left_toolbar()
{
    gluten::imgui::scoped_color toolbarBackgroundColor(ImGuiCol_ChildBg, gluten::theme::carbon_g100::field01);

    if (ImGui::BeginChild("LeftToolbar", ImVec2(leftToobarWidth, 0)))
    {
        gluten::layout buttonsLayout(gluten::layout::layout_type::top_to_bottom,
                                     gluten::element::anchor_preset::stretch_full);
        buttonsLayout.get_element_anchor().max.x += 0.1f;
        buttonsLayout.render_window();

        gluten::icon_button reviewsButton("##ReviewsButton", ICON_LC_CHART_NO_AXES_GANTT,
                                          gluten::fonts::regular_lucide_icons);
        gluten::icon_button usersButton("##UsersButton", ICON_LC_USERS, gluten::fonts::regular_lucide_icons);

        reviewsButton.set_element_min_size(ImVec2(leftToolbarButtonHeight, leftToolbarButtonHeight));
        usersButton.set_element_min_size(ImVec2(leftToolbarButtonHeight, leftToolbarButtonHeight));

        reviewsButton.set_element_scale(3.0f);
        usersButton.set_element_scale(3.0f);

        reviewsButton.set_element_hover_color(gluten::theme::carbon_g100::fieldHover01);
        usersButton.set_element_hover_color(gluten::theme::carbon_g100::fieldHover01);

        reviewsButton.set_element_active_color(gluten::theme::carbon_g100::layerActive01);
        usersButton.set_element_active_color(gluten::theme::carbon_g100::layerActive01);

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