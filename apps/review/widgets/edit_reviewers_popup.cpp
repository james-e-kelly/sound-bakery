#include "edit_reviewers_popup.h"

#include "managers/workspace_manager.h"
#include "elements/inline_user_display_element.h"

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

		m_newUsers = workspaceManager->get_users_for_review(m_review.m_reviewId);
        m_allUsers = workspaceManager->get_all_users();
		
		std::sort(m_newUsers.begin(), m_newUsers.end(), [](const auto& lhs, const auto& rhs) -> bool
			{
                return std::strcmp(lhs.m_displayName.c_str(), rhs.m_displayName.c_str()) <= 0;
			});

		std::sort(m_allUsers.begin(), m_allUsers.end(), [](const auto& lhs, const auto& rhs) -> bool
			{
                return std::strcmp(lhs.m_displayName.c_str(), rhs.m_displayName.c_str()) <= 0;
			});
	}
}

auto edit_reviewers_popup::render_popup() -> void
{
    gluten::imgui::scoped_font font(get_app()->get_font(gluten::fonts::regular_lucide_icons));

	ImGui::Dummy(ImVec2(400.0f, 0.0f));

	for (auto iter = m_newUsers.begin(); iter != m_newUsers.end(); )
	{
		gluten::imgui::scoped_id scopedId(iter->m_userId);
        ImGui::TextUnformatted(iter->m_displayName.c_str());
		ImGui::SameLine();
		if (ImGui::Button(ICON_LC_X))
		{
			iter = m_newUsers.erase(iter);
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
			for (const auto& user : m_allUsers)
			{
				if (const auto iter = std::find_if(m_newUsers.begin(), m_newUsers.end(), [newId = user.m_userId](const auto& user) -> bool
					{
						return newId == user.m_userId;
					}); iter == m_newUsers.end())
				{
					if (ImGui::Selectable(user.m_displayName.c_str()))
					{
						{
							m_newUsers.push_back(user);
							m_addingNewUser = false;
						}
					}
				}
			}
            ImGui::EndCombo();
		}
	}

	if (m_newUsers.size() != m_allUsers.size())
	{
		if (ImGui::Button(ICON_LC_PLUS))
		{
			m_addingNewUser = true;
		}
	}

	if (ImGui::Button("Save"))
	{
		if (std::shared_ptr<workspace_manager> workspaceManager = m_workspaceManager.lock())
		{
			std::vector<int64_t> userIds;
			userIds.resize(m_newUsers.size());

			std::transform(m_newUsers.begin(), m_newUsers.end(), userIds.begin(), [](const user_data& user) 
				{
					return user.m_userId;
				});

			workspaceManager->set_users_for_review(m_review.m_reviewId, userIds);
		}

		close_popup();
	}

	ImGui::SameLine();

	if (ImGui::Button("Close"))
	{
		close_popup();
	}
}