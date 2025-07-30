#include "update_review_popup.h"

#include "managers/workspace_manager.h"

auto update_review_popup::set_review_data(const review_data& reviewData) -> void
{
    m_reviewData = reviewData;
    m_reviewData.m_reviewName.copy(reviewNameBuffer, textBufferSize);
    m_reviewData.m_reviewDescription.copy(reviewDescriptionBuffer, textBufferSize);
    m_reviewData.m_reviewTaskUrl.copy(reviewUrlBuffer, textBufferSize);
}

auto update_review_popup::render_popup() -> void
{
    ImGui::InputText("Review Name", reviewNameBuffer, textBufferSize);
    ImGui::SetItemTooltip("Write the title of the review. It can be anything from \"Adding some sounds\" to "
                          "\"[TASK-1234][Level1] Add Looping Fire Sounds\"");

    ImGui::InputTextMultiline("Review Description", reviewDescriptionBuffer, textBufferSize);
    ImGui::SetItemTooltip("Give the review a description to help describe what sounds you are adding or changing");

    ImGui::InputText("Review URL", reviewUrlBuffer, textBufferSize);
    ImGui::SetItemTooltip("Add the Jira/ADO/HacknPlan task URL");

    if (ImGui::BeginCombo("Review Phase", get_review_phase_string(m_reviewData.m_reviewPhase).data()))
    {
        for (int i = 0; i < static_cast<int>(review_phase::num); ++i)
        {
            review_phase phase = static_cast<review_phase>(i);

            if (ImGui::Selectable(get_review_phase_string(phase).data()))
            {
                m_reviewData.m_reviewPhase = phase;
            }
        }

        ImGui::EndCombo();
    }
    ImGui::SetItemTooltip("Set the review phase. Useful for giving context as a temp sound does not need as much "
                          "vetting as a final pass sound");

    if (ImGui::BeginCombo("Review Quality", get_review_quality_string(m_reviewData.m_reviewQuality).data()))
    {
        for (int i = 0; i < static_cast<int>(review_quality::num); ++i)
        {
            review_quality quality = static_cast<review_quality>(i);

            if (ImGui::Selectable(get_review_quality_string(quality).data()))
            {
                m_reviewData.m_reviewQuality = quality;
            }
        }

        ImGui::EndCombo();
    }
    ImGui::SetItemTooltip("Set the review quality. The higher the quality, the more the asset may need reviewing. Your "
                          "project may vary but roughly A == \"Industry Competitive\"");

    const bool setupValid = !m_reviewData.m_reviewName.empty();

    ImGui::BeginDisabled(!setupValid);

    if (ImGui::Button("Update"))
    {
        if (std::shared_ptr<workspace_manager> workspaceManager = get_app()->get_manager_by_class<workspace_manager>())
        {
            m_reviewData.m_reviewName = reviewNameBuffer;
            m_reviewData.m_reviewDescription = reviewDescriptionBuffer;
            m_reviewData.m_reviewTaskUrl     = reviewUrlBuffer;

            workspaceManager->update_review(m_reviewData);
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