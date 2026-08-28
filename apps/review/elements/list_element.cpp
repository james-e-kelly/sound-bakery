#include "list_element.h"

#include "gluten/subsystems/animation_subsystem.h"

auto list_element::render_element(const gluten::element_render_info& renderInfo) -> bool
{
    const ImGuiID id = ImGui::GetID(m_titleText.get_text().c_str());
    gluten::imgui::scoped_id scopedId(id);

    const float hoverAlpha = gluten::animation_subsystem::animate_bool(id, false, get_element_is_hovered(), 10.0f);

    m_titleText.set_element_translation(ImVec2(hoverAlpha * gluten::theme::space04, 0.0f));
    m_descriptionText.set_element_translation(ImVec2(hoverAlpha * gluten::theme::space08, 0.0f));

    m_titleText.render(renderInfo.elementBox);
    m_descriptionText.render(renderInfo.elementBox);
    m_detailText.render(renderInfo.elementBox);

    const bool pressed = m_projectButton.render(renderInfo.elementBox);

    if (pressed)
    {
        gluten::animation_subsystem::clear(id);
    }

    //if (gluten::imgui::scoped_context_menu contextMenu{"Project Context"})
    //{
    //    if (std::shared_ptr<workspace_manager> workspaceManager = gluten::app::get()->get_manager_by_class<workspace_manager>())
    //    {
    //        if (workspaceManager->get_user_privileges() == user_privileges::admin)
    //        {
    //            if (ImGui::Selectable("Delete Project"))
    //            {
    //                workspaceManager->delete_project(m_projectTitleText.get_text());
    //            }
    //        }
    //    }
    //}

    return pressed;
}
