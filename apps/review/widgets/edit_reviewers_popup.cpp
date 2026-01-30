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

	render_reviewers();

	ImGui::BeginDisabled(!m_newUsers.has_value());

	if (ImGui::Button("Save"))
	{
		if (std::shared_ptr<workspace_manager> workspaceManager = m_workspaceManager.lock())
		{
			std::vector<int64_t> userIds;
			userIds.resize(m_newUsers.value().size());

			std::transform(m_newUsers.value().begin(), m_newUsers.value().end(), userIds.begin(), [](const user_data& user) 
				{
					return user.m_userId;
				});

			workspaceManager->set_review_users(m_review.m_reviewId, userIds);
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