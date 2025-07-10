#include "create_project_popup.h"

#include "managers/workspace_manager.h"

auto create_project_popup::render_popup() -> void
{
    ImGui::SetWindowFontScale(1.5f);

    ImGui::InputTextWithHint("Project Name", "My New Project", projectNameBuffer, textBufferSize);

    ImGui::InputTextWithHint("Project Description", "2D platformer metroidvania", projectDescriptionBuffer, textBufferSize);

    const std::string projectName        = projectNameBuffer;
    const std::string projectDescription = projectDescriptionBuffer;

    const bool setupValid = !projectName.empty();

    ImGui::BeginDisabled(!setupValid);

    if (ImGui::Button("Create"))
    {
        if (std::shared_ptr<workspace_manager> workspaceManager = get_app()->get_manager_by_class<workspace_manager>())
        {
            workspaceManager->create_project(projectName, projectDescription);
        }

        close_popup();
    }

    ImGui::EndDisabled();

    ImGui::SameLine();

    if (ImGui::Button("Cancel"))
    {
        close_popup();
    }

    ImGui::SetWindowFontScale(1.0f);
}