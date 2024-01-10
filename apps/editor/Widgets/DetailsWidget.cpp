#include "DetailsWidget.h"

#include "App/App.h"
#include "Managers/ProjectManager.h"
#include "Utils/PropertyDrawer.h"
#include "Utils/MethodDrawer.h"

#include "sound_bakery/node/node.h"
#include "sound_bakery/node/container/sound_container.h"
#include "imgui.h"

void DetailsWidget::Render()
{
    Widget::Render();

    if (ImGui::Begin("Details", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (ProjectManager* const projectManager = GetApp()->GetProjectManager())
        {
            Selection& selection = projectManager->GetSelection();
            if (SB::Core::Object* selected = selection.GetSelected())
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