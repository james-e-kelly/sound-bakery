#include "root_widget.h"

#include "app/app.h"
#include "managers/app_manager.h"
#include "managers/project_manager.h"
#include "IconsFontAwesome6.h"
#include "IconsFontaudio.h"
#include "imgui.h"

#include "sound_bakery/editor/project/project.h"

void root_widget::render_menu()
{
    static bool showMenu = false;
    bool showAbout       = false;

    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem(ICON_FA_FILE " New...", "Ctrl+N"))
            {
                get_app()->get_manager_by_class<app_manager>()->create_new_project();
            }

            if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN " Open...", "Ctrl+O"))
            {
                get_app()->get_manager_by_class<app_manager>()->open_project();
            }

            if (get_app()->get_manager_by_class<project_manager>())
            {
                ImGui::Separator();

                ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
                if (ImGui::MenuItem(ICON_FAD_SAVE " Save", "Ctrl+S"))
                {
                    get_app()->get_manager_by_class<project_manager>()->save_project();
                }

                if (ImGui::MenuItem(ICON_FAD_SAVEAS " Save As...",
                                    "Shift+Ctrl+S"))
                {
                }
                ImGui::PopFont();
            }

            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4"))
            {
                get_app()->request_exit();
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Options"))
        {

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Actions"))
        {

            if (ImGui::MenuItem("Open Demo Window", nullptr, &showMenu))
            {
            }

            if (ImGui::MenuItem("Convert Files", nullptr, nullptr))
            {
                sbk::engine::system::get_project()->encode_all_media();
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help"))
        {
            if (ImGui::MenuItem("View Documentation"))
            {
            }

            ImGui::Separator();
            if (ImGui::MenuItem("About Atlas"))
            {
                showAbout = true;
            }

            ImGui::EndMenu();
        }

    }

    if (showAbout)
    {
        ImGui::OpenPopup("About");
    }

    if (ImGui::BeginPopupModal("About", NULL,
                               ImGuiWindowFlags_AlwaysAutoResize))
    {
        static std::string aboutText(
            "Atlas is an open source audio middleware alternatie to Wwise and "
            "FMOD.\nWritten by James Kelly.");

        auto windowWidth = ImGui::GetWindowSize().x;
        auto textWidth   = ImGui::CalcTextSize(aboutText.c_str()).x;

        ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
        ImGui::Text("%s", aboutText.c_str());

        if (ImGui::Button("Close"))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    if (showMenu)
    {
        //        ImGui::ShowUserGuide();
        ImGui::ShowDemoWindow();
        //        ImGui::ShowAboutWindow();
        //        ImGui::ShowDebugLogWindow();
    }
}