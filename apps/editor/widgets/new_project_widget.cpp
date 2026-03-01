#include "new_project_widget.h"

#include "nfd.h"
#include "app/app.h"
#include "gluten/theme/carbon/carbon_theme_g100.h"
#include "gluten/elements/button.h"
#include "gluten/elements/layouts/layout.h"
#include "gluten/utils/imgui_util_structures.h"

auto new_project_widget::open_new_project_popup() -> void 
{ 
    set_visibile(true); 
    ImGui::OpenPopup(get_widget_name().data());
}

auto new_project_widget::close_new_project_popup() -> void 
{
    ImGui::CloseCurrentPopup();
    set_visibile(false);
}

void new_project_widget::start_implementation() {
}

void new_project_widget::render_implementation()
{
    if (!ImGui::IsPopupOpen(get_widget_name().data()))
    {
        ImGui::OpenPopup(get_widget_name().data());
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(800, 250), ImGuiCond_Appearing);

    gluten::imgui::scoped_color inputTextBackground(ImGuiCol_FrameBg, gluten::theme::carbon_g100::layer01);

    if (ImGui::BeginPopupModal(get_widget_name().data(), nullptr, ImGuiWindowFlags_NoResize))
    {
        static char projectName[512];
        static char projectFolder[512];

        static std::string projectNameString;
        static std::string projectFolderString;
        
        ImGui::NewLine();

        if (ImGui::InputText("Project Name", projectName, 512))
        {
            projectNameString = projectName;
        }

        if (ImGui::InputText("Project Folder", projectFolder, 512))
        {
            projectFolderString = projectFolder;

        }
        ImGui::SameLine();
        if (ImGui::Button("...###OpenProjectFolder"))
        {
            NFD_Init();

            nfdchar_t* outPath = NULL;

        retry:
            nfdresult_t pickFolderResult = NFD_PickFolder(&outPath, std::filesystem::current_path().string().c_str());

            switch (pickFolderResult)
            {
                case NFD_OKAY:
                    projectFolderString = outPath;
                    std::strcpy(projectFolder, projectFolderString.c_str());
                    break;

                case NFD_CANCEL:
                    break;

                case NFD_ERROR:
                default:
                    goto retry;
                    break;
            }

            NFD_FreePath(outPath);
            NFD_Quit();
        }

        gluten::layout bottomButtonsLayout(gluten::element::anchor_preset::stretch_bottom);
        bottomButtonsLayout.get_element_anchor().min.y = 0.7f;
        bottomButtonsLayout.set_element_window_padding();
        bottomButtonsLayout.render_window();

        gluten::button cancelButton("Cancel", false, gluten::element::anchor_preset::stretch_full);
        gluten::button createButton("Create", false, gluten::element::anchor_preset::stretch_full);

        cancelButton.set_element_frame_padding();
        createButton.set_element_frame_padding();

        if (bottomButtonsLayout.render_layout_element_percent_horizontal(&cancelButton, 0.5f))
        {
            close_new_project_popup();
        }

        const bool validProjectCreation = !projectNameString.empty() && std::filesystem::exists(projectFolderString);

        ImGui::BeginDisabled(!validProjectCreation);

        if (bottomButtonsLayout.render_layout_element_percent_horizontal(&createButton, 0.5f))
        {
            std::filesystem::directory_entry projectDirectory(projectFolderString);

            if (std::filesystem::exists(projectDirectory))
            {
                static_cast<editor_app*>(get_app())->create_and_open_project(projectDirectory, projectNameString);
                close_new_project_popup();
            }
        }

        ImGui::EndDisabled();

        ImGui::EndPopup();
    }
}
