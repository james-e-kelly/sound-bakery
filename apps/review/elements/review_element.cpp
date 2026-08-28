#include "review_element.h"

#include "managers/workspace_manager.h"

auto review_element::render_element(const gluten::element_render_info& renderInfo) -> bool
{
    gluten::imgui::scoped_id id(ImGui::GetID(m_reviewId));

    const bool pressed = list_element::render_element(renderInfo);

    if (gluten::imgui::scoped_context_menu contextMenu{"Review Context"})
    {
        if (ImGui::Selectable("Delete Review"))
        {
            if (std::shared_ptr<workspace_manager> workspaceManager =
                    gluten::app::get()->get_manager_by_class<workspace_manager>())
            {
                workspaceManager->delete_review(m_reviewId);
            }
        }
    }

    return pressed;
}