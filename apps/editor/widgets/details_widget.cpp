#include "details_widget.h"

#include "app/app.h"
#include "imgui.h"
#include "managers/project_manager.h"
#include "sound_bakery/node/container/sound_container.h"
#include "sound_bakery/node/node.h"
#include "utils/method_drawer.h"
#include "utils/property_drawer.h"

void details_widget::render()
{
    widget::render();

    if (ImGui::Begin("Details", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (project_manager* const projectManager = get_app()->get_manager_by_class<project_manager>())
        {
            selection& selection = projectManager->get_selection();
            if (sbk::core::object* selected = selection.get_selected())
            {
                property_drawer::draw_object(selected->getType(), selected);
                ImGui::Separator();
                method_drawer::draw_object(selected->getType(), selected);
            }
            else
            {
                ImGui::TextUnformatted("Nothing Selected");
            }
        }
    }

    ImGui::End();
}