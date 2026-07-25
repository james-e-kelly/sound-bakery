#include "edit_reviewers.h"

#include "managers/workspace_manager.h"

auto edit_reviewers::render_reviewers() -> void
{
    const std::shared_ptr<workspace_manager> workspaceManager = gluten::app::get()->get_manager_by_class<workspace_manager>();

    if (!workspaceManager || (m_reviewId == 0 && m_projectId == 0))
    {
        return;
    }

    gluten::imgui::scoped_font iconFont(gluten::app::get()->get_font(gluten::fonts::regular_lucide_icons));

    const typename workspace_manager::global_cache_type<user_data>::cache_result& allUsers = workspaceManager->get_all_users();

    if (!m_newUsers.has_value())
    {
        if (m_reviewId > 0)
        {
            const typename workspace_manager::default_cache_type<reviewer_data>::cache_result& reviewers = workspaceManager->get_review_users(m_reviewId);

            if (reviewers.m_state == gluten::cache_state::has_data)
            {
                m_newUsers.value().resize(reviewers.m_cache.size());
                std::transform(reviewers.m_cache.begin(), reviewers.m_cache.end(), m_newUsers.value().begin(), [](const auto& reviewUser)
                               { return reviewUser; });
            }
            else if (reviewers.m_state == gluten::cache_state::no_data)
            {
                m_newUsers = std::vector<user_data>();
            }
        }
        else if (m_projectId > 0)
        {
            const typename workspace_manager::default_cache_type<user_data>::cache_result& projectUsers = workspaceManager->get_project_users(m_projectId);

            if (projectUsers.m_state == gluten::cache_state::has_data)
            {
                m_newUsers = projectUsers.m_cache;
            }
            else if (projectUsers.m_state == gluten::cache_state::no_data)
            {
                m_newUsers = std::vector<user_data>();
            }
        }
    }

    if (m_newUsers.has_value())
    {
        for (auto iter = m_newUsers.value().begin(); iter != m_newUsers.value().end();)
        {
            gluten::imgui::scoped_id scopedId(iter->m_userId);
            ImGui::TextUnformatted(iter->m_displayName.c_str());
            ImGui::SameLine();
            if (ImGui::Button(ICON_LC_X))
            {
                iter = m_newUsers.value().erase(iter);
            }
            else
            {
                ++iter;
            }
            ImGui::SetItemTooltip("Remove Reviewer");
        }

        if (m_addingNewUser)
        {
            if (ImGui::BeginCombo("New Reviewer", nullptr))
            {
                for (const auto& user : allUsers.m_cache)
                {
                    if (const auto iter = std::find_if(m_newUsers.value().begin(), m_newUsers.value().end(), [newId = user.m_userId](const auto& user) -> bool
                                                       { return newId == user.m_userId; });
                        iter == m_newUsers.value().end())
                    {
                        if (ImGui::Selectable(user.m_displayName.c_str()))
                        {
                            m_newUsers.value().push_back(user);
                            m_addingNewUser = false;
                        }
                    }
                }
                ImGui::EndCombo();
            }
        }

        if (m_newUsers.value().size() != allUsers.m_cache.size())
        {
            if (ImGui::Button(ICON_LC_PLUS))
            {
                m_addingNewUser = true;
            }
            ImGui::SetItemTooltip("Add Reviewer");
        }
    }
    else
    {
        gluten::loading_spinner reviewersLoading;
        reviewersLoading.render_cursor();
    }
}