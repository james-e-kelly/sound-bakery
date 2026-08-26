#include "update_review_popup.h"

#include "data/user_data.h"
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

    ImGui::InputTextMultiline("Review Description", reviewDescriptionBuffer, textBufferSize, ImVec2(0, 0), ImGuiInputTextFlags_WordWrap);
    ImGui::SetItemTooltip("Give the review a description to help describe what sounds you are adding or changing");

    ImGui::InputText("Review URL", reviewUrlBuffer, textBufferSize);
    ImGui::SetItemTooltip("Add the Jira/ADO/HacknPlan task URL");

    const std::shared_ptr<workspace_manager> workspaceManager = get_app()->get_manager_by_class<workspace_manager>();

    const typename workspace_manager::global_cache_type<user_data>::cache_result& allUsers       = workspaceManager->get_all_users();
    const typename workspace_manager::default_cache_type<reviewer_data>::cache_result& reviewers = workspaceManager->get_review_users(m_reviewData.m_reviewId);

    if (!m_newReviewers.has_value() && reviewers.m_state == gluten::cache_state::has_data)
    {
        m_newReviewers = reviewers.m_cache;
    }

    gluten::imgui::scoped_font iconFont(gluten::app::get()->get_font(gluten::fonts::regular_lucide_icons));

    if (m_newReviewers.has_value())
    {
        for (auto iter = m_newReviewers.value().begin(); iter != m_newReviewers.value().end();)
        {
            gluten::imgui::scoped_id scopedId(iter->m_userId);
            ImGui::TextUnformatted(iter->m_displayName.c_str());
            ImGui::SameLine();
            if (ImGui::Button(ICON_LC_X))
            {
                iter = m_newReviewers.value().erase(iter);
            }
            else
            {
                ++iter;
            }
        }

        if (m_addingNewUser)
        {
            if (ImGui::BeginCombo("New Reviewer", nullptr))
            {
                for (const auto& user : allUsers.m_cache)
                {
                    if (const auto iter = std::find_if(m_newReviewers.value().begin(), m_newReviewers.value().end(),
                                                       [newId = user.m_userId](const auto& user) -> bool
                                                       { return newId == user.m_userId; });
                        iter == m_newReviewers.value().end())
                    {
                        if (ImGui::Selectable(user.m_displayName.c_str()))
                        {
                            m_newReviewers.value().push_back(user);
                            m_addingNewUser = false;
                        }
                    }
                }
                ImGui::EndCombo();
            }
        }

        if (m_newReviewers.value().size() != allUsers.m_cache.size())
        {
            if (ImGui::Button(ICON_LC_PLUS))
            {
                m_addingNewUser = true;
            }
        }
    }
    else
    {
        gluten::loading_spinner reviewersLoading;
        reviewersLoading.render_cursor();
    }

    const bool setupValid = !m_reviewData.m_reviewName.empty();

    ImGui::BeginDisabled(!setupValid);

    if (ImGui::Button("Update"))
    {
        if (std::shared_ptr<workspace_manager> workspaceManager = get_app()->get_manager_by_class<workspace_manager>())
        {
            m_reviewData.m_reviewName        = reviewNameBuffer;
            m_reviewData.m_reviewDescription = reviewDescriptionBuffer;
            m_reviewData.m_reviewTaskUrl     = reviewUrlBuffer;

            workspaceManager->update_review(m_reviewData);

            std::vector<int64_t> userIds;
            userIds.resize(m_newReviewers.value().size());

            std::transform(m_newReviewers.value().begin(),
                           m_newReviewers.value().end(), userIds.begin(),
                           [](const user_data& user)
                           { return user.m_userId; });

            workspaceManager->set_review_users(m_reviewData.m_reviewId, userIds);
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