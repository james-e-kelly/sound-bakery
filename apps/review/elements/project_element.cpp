#include "project_element.h"

#include "gluten/subsystems/animation_subsystem.h"
#include "managers/workspace_manager.h"

auto project_element::render_element(const gluten::element_render_info& renderInfo) -> bool
{
    const bool pressed = list_element::render_element(renderInfo);
    
    if (gluten::imgui::scoped_context_menu contextMenu{"Project Context"})
    {
        if (std::shared_ptr<workspace_manager> workspaceManager = gluten::app::get()->get_manager_by_class<workspace_manager>())
        {
            if (workspaceManager->get_user_privileges() == user_privileges::admin)
            {
                if (ImGui::Selectable("Delete Project"))
                {
                    workspaceManager->delete_project(m_titleText.get_text());
                }
            }
        }
    }

    return pressed;
}
