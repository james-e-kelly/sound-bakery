#include "root_widget.h"

#include "gluten/utils/imgui_util_structures.h"
#include "IconsFontAwesome6.h"
#include "IconsFontaudio.h"
#include "app/app.h"
#include "imgui.h"
#include "managers/app_manager.h"
#include "managers/project_manager.h"
#include "sound_bakery/editor/project/project.h"

void root_widget::render_menu()
{
    static bool showMenu  = false;
    static bool showAbout = false;

    gluten::imgui::scoped_font fontAwesomeFont(get_app()->get_font(gluten::fonts::regular_font_awesome));

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

                if (ImGui::MenuItem(ICON_FAD_SAVEAS " Save As...", "Shift+Ctrl+S"))
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
                ImGuiContext* const context = ImGui::GetCurrentContext();
                if (context->PlatformIO.Platform_OpenInShellFn != NULL)
                {
                    context->PlatformIO.Platform_OpenInShellFn(context, "https://soundbakery.jameskelly.audio");
                }
            }

            ImGui::Separator();
            if (ImGui::MenuItem("About " SBK_PRODUCT_NAME "..."))
            {
                showAbout = true;
            }

            ImGui::EndMenu();
        }
    }

    if (showAbout)
    {
        render_about_window(showAbout);
    }

    if (showMenu)
    {
        ImGui::ShowDemoWindow();
    }

    gluten::root_widget::render_menu();
}

void root_widget::render_about_window(bool& showAbout)
{
    if (ImGui::Begin("About " SBK_PRODUCT_NAME, &showAbout, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("%s %s", SBK_PRODUCT_NAME, SBK_VERSION_STRING_FULL);

        ImGui::TextLinkOpenURL("Homepage", "https://github.com/james-e-kelly/sound-bakery");
        ImGui::SameLine();
        ImGui::TextLinkOpenURL("Documentation", "https://soundbakery.jameskelly.audio");
        ImGui::SameLine();
        ImGui::TextLinkOpenURL("Releases", "https://github.com/james-e-kelly/sound-bakery/releases");

        ImGui::Separator();
        ImGui::Text(SBK_PRODUCT_DESCRIPTION);
        ImGui::Text(SBK_PRODUCT_NAME " is licensed under the MIT License, see LICENSE for more information.");

        ImGui::End();
    }
}
