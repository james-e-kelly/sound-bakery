#include "DetailsWidget.h"

#include "App/App.h"
#include "Managers/ProjectManager.h"
#include "Utils/MethodDrawer.h"
#include "Utils/PropertyDrawer.h"
#include "imgui.h"
#include "sound_bakery/node/container/sound_container.h"
#include "sound_bakery/node/node.h"

void DetailsWidget::Render()
{
    widget::render();

    if (ImGui::Begin("Details", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (ProjectManager* const projectManager =
                get_app()->GetProjectManager())
        {
            Selection& selection = projectManager->GetSelection();
            if (sbk::core::object* selected = selection.GetSelected())
            {
                PropertyDrawer::DrawObject(selected->getType(), selected);
                ImGui::Separator();
                MethodDrawer::DrawObject(selected->getType(), selected);
            }
            else
            {
                ImGui::TextUnformatted("Nothing Selected");
            }
        }
    }

    ImGui::End();
}