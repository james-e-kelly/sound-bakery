#include "details_widget.h"

#include "app/app.h"
#include "gluten/utils/imgui_util_functions.h"
#include "gluten/utils/imgui_util_structures.h"
#include "imgui.h"
#include "managers/project_manager.h"
#include "sound_bakery/node/container/sound_container.h"
#include "sound_bakery/node/node.h"
#include "utils/method_drawer.h"
#include "utils/property_drawer.h"

void details_widget::render()
{
    widget::render();

    const ImVec2 itemSpacing = ImGui::GetStyle().ItemSpacing;

    gluten::imgui::scoped_style_stack detailsWindowPadding(ImGuiStyleVar_WindowPadding, ImVec2(0, 0),
                                                           ImGuiStyleVar_ItemSpacing, ImVec2(itemSpacing.x, 0));

    if (ImGui::Begin("Details", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (project_manager* const projectManager = get_app()->get_manager_by_class<project_manager>())
        {
            if (ImGui::CollapsingHeader("Details", ImGuiTreeNodeFlags_DefaultOpen))
            {
                selection& selection = projectManager->get_selection();
                if (sbk::core::object* selected = selection.get_selected())
                {
                    gluten::imgui::indent_cursor();
                    property_drawer::draw_object(selected->getType(), selected);
                    gluten::imgui::indent_cursor();
                    method_drawer::draw_object(selected->getType(), selected);
                }
            }
        }
    }

    ImGui::End();
}