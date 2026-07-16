#include "intro_widget.h"

#include "app/review_app.h"
#include "gluten/elements/layouts/layout.h"
#include "gluten/elements/text.h"
#include "gluten/elements/button.h"
#include "imgui_stdlib.h"
#include "managers/intro_manager.h"

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
    ImGui::BeginDisabled(static_cast<bool>(m_testConnectionResult));

	gluten::layout centerColumnLayout(gluten::layout::layout_type::top_to_bottom, gluten::element::anchor_preset::stretch_center);
    centerColumnLayout.get_element_anchor().maxOffset.x = 800;
    centerColumnLayout.get_element_anchor().minOffset.x = -400;
    centerColumnLayout.set_element_content_scale(1.5f);

	centerColumnLayout.render_window();

	gluten::text welcomeText("Welcome to Sound Check!", ImVec2(0.5f, 0.5f), gluten::element::anchor_preset::center_middle);
	gluten::text secondText("To get started, either set up a server or connect to one.", ImVec2(0.5f, 0.5f), gluten::element::anchor_preset::center_middle);
	gluten::text clientHeader("Connect To Server", ImVec2(0.5f, 0.5f), gluten::element::anchor_preset::center_middle);
	gluten::text serverHeader("Set Up Server", ImVec2(0.5f, 0.5f), gluten::element::anchor_preset::center_middle);
	gluten::text ipAddressText("IP Address", ImVec2(0.5f, 0.5f), gluten::element::anchor_preset::center_middle);

	welcomeText.set_font(gluten::fonts::title);
    clientHeader.set_font(gluten::fonts::title);
    serverHeader.set_font(gluten::fonts::title);

	welcomeText.set_element_content_scale(1.25f);
    clientHeader.set_element_content_scale(1.25f);
    serverHeader.set_element_content_scale(1.25f);

	centerColumnLayout.render_layout_element_pixels_vertical(nullptr, 50.0f);
	centerColumnLayout.render_layout_element_pixels_vertical(&welcomeText, 50.0f);
	centerColumnLayout.render_layout_element_pixels_vertical(&secondText, 50.f);
	centerColumnLayout.render_layout_element_pixels_vertical(nullptr, 50.0f);

	gluten::layout twoColumnLayout(gluten::layout::layout_type::left_to_right, gluten::element::anchor_preset::stretch_full);
    twoColumnLayout.set_element_frame_padding();

	gluten::button openButton("Open Review Database File...", false, gluten::element::anchor_preset::center_top);
	gluten::button createButton("Create Review Database File...", false, gluten::element::anchor_preset::center_top);
	gluten::button connectButton("Connect To Server...", false, gluten::element::anchor_preset::center_top);

	openButton.set_element_alignment(ImVec2(0.5f, 0.f));
	createButton.set_element_alignment(ImVec2(0.5f, 0.f));
	connectButton.set_element_alignment(ImVec2(0.5f, 0.f));

	centerColumnLayout.render_layout_element_pixels_vertical(&clientHeader, 50.0f);

	ImGui::SetCursorScreenPos(centerColumnLayout.get_current_layout_pos());
    ImGui::PushItemWidth(centerColumnLayout.get_element_rect().GetWidth());

	static char textBuffer[512];
    ImGui::InputTextWithHint("IP Address", "172.0.0.1", textBuffer, 512);

	centerColumnLayout.render_layout_element_pixels_vertical(nullptr, 60.0f);
	if (centerColumnLayout.render_layout_element_pixels_vertical(&connectButton, 50.0f))
	{
        m_testConnectionResult = review_app_test_connect::test_server_connection(textBuffer);
	}
	centerColumnLayout.render_layout_element_pixels_vertical(nullptr, 60.0f);

	centerColumnLayout.render_layout_element_pixels_vertical(&serverHeader, 50.0f);
    
	centerColumnLayout.render_layout_element_pixels_vertical(&twoColumnLayout, 200.0f);

	if (twoColumnLayout.render_layout_element_percent_horizontal(&openButton, 0.5f))
	{
        const std::filesystem::path workspaceFile = gluten::app::open_select_file_dialog("workspace", "workspace");

		if (std::filesystem::exists(workspaceFile))
		{
			review_app::set_up_server(workspaceFile);
		}
	}

	if (twoColumnLayout.render_layout_element_percent_horizontal(&createButton, 0.5f))
	{
		if (m_createWorkspacePopup)
		{
            m_createWorkspacePopup->open_popup();
		}
	}

	if (m_testConnectionResult && m_testConnectionResult.status() == concurrencpp::result_status::value)
	{
		if (m_testConnectionResult.get())
		{
            review_app::set_up_client(textBuffer);
		}
	}

	ImGui::EndDisabled();
}

auto create_workspace_popup::render_popup() -> void
{
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
        const std::filesystem::path workspaceFile = std::filesystem::path(workspaceDirectory) / (workspaceName + ".workspace");
		review_app::set_up_server(workspaceFile);

        close_popup();
	}

	ImGui::EndDisabled();

	ImGui::SameLine();

	if (ImGui::Button("Cancel"))
	{
        close_popup();
	}
}