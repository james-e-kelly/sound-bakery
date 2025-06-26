#include "intro_widget.h"

#include "gluten/elements/layouts/layout.h"
#include "gluten/elements/text.h"
#include "gluten/elements/button.h"
#include "imgui_stdlib.h"

#include "managers/workspace_manager.h"

auto intro_widget::start_implementation() -> void
{
    m_createWorkspacePopup = add_child_widget<create_workspace_popup>(false);

	ImGuiWindowClass windowClass;
    windowClass.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoTabBar;

    set_window_class(windowClass);
}

auto intro_widget::render_window_implementation() -> void
{
	gluten::layout centerColumnLayout(gluten::layout::layout_type::top_to_bottom, gluten::element::anchor_preset::stretch_center);
    centerColumnLayout.get_element_anchor().maxOffset.x = 800;
    centerColumnLayout.get_element_anchor().minOffset.x = -400;
    centerColumnLayout.set_element_scale(1.5f);

	centerColumnLayout.render_window();

	gluten::text welcomeText("Welcome to Sound Check!", ImVec2(0.5f, 0.5f), gluten::element::anchor_preset::center_middle);
	gluten::text secondText("To get started, create or open a workspace.", ImVec2(0.5f, 0.5f), gluten::element::anchor_preset::center_middle);

	welcomeText.set_element_scale(1.5f);

	centerColumnLayout.render_layout_element_pixels_vertical(&welcomeText, 100.f);
	centerColumnLayout.render_layout_element_pixels_vertical(&secondText, 50.f);

	gluten::layout twoColumnLayout(gluten::layout::layout_type::left_to_right, gluten::element::anchor_preset::stretch_full);
    twoColumnLayout.set_element_frame_padding();

    centerColumnLayout.render_layout_element_remaining(&twoColumnLayout);

	gluten::button openButton("Open...", false, gluten::element::anchor_preset::center_top);
	gluten::button createButton("Create...", false, gluten::element::anchor_preset::center_top);

	openButton.set_element_alignment(ImVec2(0.5f, 0.f));
	createButton.set_element_alignment(ImVec2(0.5f, 0.f));

	if (twoColumnLayout.render_layout_element_percent_horizontal(&openButton, 0.5f))
	{
        const std::filesystem::path workspaceFile = gluten::app::open_select_file_dialog("workspace", "workspace");

		if (std::shared_ptr<workspace_manager> workspaceManager = get_app()->get_manager_by_class<workspace_manager>())
		{
            workspaceManager->open_workspace(workspaceFile);
		}
	}

	if (twoColumnLayout.render_layout_element_percent_horizontal(&createButton, 0.5f))
	{
		if (m_createWorkspacePopup)
		{
            m_createWorkspacePopup->open_popup();
		}
	}
}

auto create_workspace_popup::render_popup() -> void
{
    ImGui::SetWindowFontScale(1.5f);

	static constexpr std::size_t textBufferSize = 512;

	static char workspaceNameBuffer[textBufferSize];
	static char workspaceDirectoryBuffer[textBufferSize];

	ImGui::InputTextWithHint("Workspace Name", "My New Workspace", workspaceNameBuffer, textBufferSize);

	ImGui::InputTextWithHint("Workspace Directory", "Shared Folder", workspaceDirectoryBuffer, textBufferSize);
    ImGui::SameLine();
	if (ImGui::Button("Select..."))
	{
        const std::string directory = gluten::app::open_select_folder_dialog().string();

		if (std::filesystem::exists(directory))
		{
            directory.copy(workspaceDirectoryBuffer, textBufferSize);
		}
	}

	const std::string workspaceName = workspaceNameBuffer;
	const std::string workspaceDirectory = workspaceDirectoryBuffer;

	const bool setupValid = !workspaceName.empty() && !workspaceDirectory.empty();

	ImGui::BeginDisabled(!setupValid);

	if (ImGui::Button("Create"))
	{
        if (std::shared_ptr<workspace_manager> workspaceManager = get_app()->get_manager_by_class<workspace_manager>())
		{
            workspaceManager->create_workspace(workspaceName, workspaceDirectory);
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