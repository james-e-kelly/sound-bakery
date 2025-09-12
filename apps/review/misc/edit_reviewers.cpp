#include "edit_reviewers.h"

#include "managers/workspace_manager.h"

auto edit_reviewers::render_reviewers() -> void
{
    const std::shared_ptr<workspace_manager> workspaceManager = gluten::app::get()->get_manager_by_class<workspace_manager>();

	if (!workspaceManager)
	{
		return;
	}

	gluten::imgui::scoped_font iconFont(gluten::app::get()->get_font(gluten::fonts::regular_lucide_icons));

	const typename workspace_manager::global_cache_type<user_data>::cache_result& allUsers = workspaceManager->get_all_users();
	const typename workspace_manager::default_cache_type<reviewer_data>::cache_result& reviewers = workspaceManager->get_review_users(m_reviewId);

	if (!m_newReviewers.has_value() && reviewers.m_state == gluten::cache_state::has_data)
	{
        m_newReviewers = reviewers.m_cache;
	}

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
			ImGui::SetItemTooltip("Remove Reviewer");
		}

		if (m_addingNewUser)
		{
			if (ImGui::BeginCombo("New Reviewer", nullptr))
			{
				for (const auto& user : allUsers.m_cache)
				{
					if (const auto iter = std::find_if(m_newReviewers.value().begin(), m_newReviewers.value().end(), [newId = user.m_userId](const auto& user) -> bool
						{
							return newId == user.m_userId;
						}); iter == m_newReviewers.value().end())
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
			ImGui::SetItemTooltip("Add Reviewer");
		}
	}
	else
	{
		gluten::loading_spinner reviewersLoading;
        reviewersLoading.render_cursor();
	}
}