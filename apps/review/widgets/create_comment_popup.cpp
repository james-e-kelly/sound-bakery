#include "create_comment_popup.h"

#include "managers/workspace_manager.h"

auto create_comment_popup::render_popup() -> void
{
    bool wantsToAdd = false;

    if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Enter))
    {
        wantsToAdd = true;
    }

    if (ImGui::IsWindowFocused() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0))
    {
        ImGui::SetKeyboardFocusHere(0);
    }

    ImGui::InputTextMultiline("##Comment", commentBuffer, textBufferSize);

    const bool setupValid = commentBuffer[0] != '\0';

    ImGui::BeginDisabled(!setupValid);

    if (ImGui::Button("Create") || wantsToAdd)
    {
        if (std::shared_ptr<workspace_manager> workspaceManager = get_app()->get_manager_by_class<workspace_manager>())
        {
            new_comment_data newComment;
            newComment.m_comment = commentBuffer;
            newComment.m_timeStart = m_videoPosition;
            newComment.m_reviewId  = m_reviewId;
            newComment.m_fileId    = m_fileId;

            workspaceManager->create_comment(newComment);
        }

        close_popup();
    }

    ImGui::EndDisabled();

    ImGui::SameLine();

    if (ImGui::Button("Cancel"))
    {
        close_popup();
    }
}