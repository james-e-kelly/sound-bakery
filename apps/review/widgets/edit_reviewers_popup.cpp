#include "edit_reviewers_popup.h"

#include "managers/workspace_manager.h"

namespace
{
    constexpr const char* g_addReviewerPopupName = "Add Reviewer";
}

auto edit_reviewers_popup::start_implementation() -> void
{
	gluten::popup_widget::start_implementation();

	if (std::shared_ptr<workspace_manager> workspaceManager = get_app()->get_manager_by_class<workspace_manager>())
	{
		m_workspaceManager = workspaceManager;
	}
}

auto edit_reviewers_popup::render_popup() -> void
{
    gluten::imgui::scoped_font font(get_app()->get_font(gluten::fonts::regular_lucide_icons));

	ImGui::Dummy(ImVec2(400.0f, 0.0f));

	const std::shared_ptr<workspace_manager> workspaceManager = m_workspaceManager.lock();

	if (!workspaceManager)
	{
		return;
	}

	const typename workspace_manager::users_cache_type<user_data>::cache_result& allUsers = workspaceManager->get_all_users();
	const typename workspace_manager::default_cache_type<reviewer_data>::cache_result& reviewers = workspaceManager->get_users_for_review(m_review.m_reviewId);

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
		}
	}
	else
	{
		gluten::loading_spinner reviewersLoading;
        reviewersLoading.render_cursor();
	}

	ImGui::BeginDisabled(!m_newReviewers.has_value());

	if (ImGui::Button("Save"))
	{
		if (std::shared_ptr<workspace_manager> workspaceManager = m_workspaceManager.lock())
		{
			std::vector<int64_t> userIds;
			userIds.resize(m_newReviewers.value().size());

			std::transform(m_newReviewers.value().begin(), m_newReviewers.value().end(), userIds.begin(), [](const user_data& user) 
				{
					return user.m_userId;
				});

			workspaceManager->set_users_for_review(m_review.m_reviewId, userIds);
		}

		close_popup();
	}

	ImGui::EndDisabled();

	ImGui::SameLine();

	if (ImGui::Button("Close"))
	{
		close_popup();
	}
}