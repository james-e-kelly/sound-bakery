#include "workspace_widget.h"

#include "managers/workspace_manager.h"

auto workspace_widget::render_window_implementation() -> void
{
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