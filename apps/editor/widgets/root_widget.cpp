#include "root_widget.h"

#include "sound_bakery/editor/project/project.h"

#include "IconsFontAwesome6.h"
#include "IconsFontaudio.h"
#include "IconsLucide.h"
#include "app/app.h"
#include "gluten/utils/imgui_util_structures.h"
#include "imgui.h"
#include "managers/app_manager.h"
#include "managers/project_manager.h"
#include "widgets/audio_meter_widget.h"
#include "widgets/details_widget.h"
#include "widgets/play_controls_widget.h"
#include "widgets/project_explorer_widget.h"

auto root_widget::start_implementation() -> void
{
    gluten::root_widget::start_implementation();

    add_layout(gluten::widget_layout("Designer", [this](gluten::dockspace_refresh& refresh)
                                     {
            refresh.split_three_columns_large_main();

            ImGuiID bottomLeftID = 0;
            ImGui::DockBuilderSplitNode(refresh.leftColumnID, ImGuiDir_Down, 0.2f, &bottomLeftID, &refresh.leftColumnID);

            refresh.assign_widget_to_node(rttr::type::get<project_explorer_widget>(), refresh.leftColumnID);
            refresh.assign_widget_to_node(rttr::type::get<details_widget>(), refresh.centerColumnID);
            refresh.assign_widget_to_node(rttr::type::get<audio_meter_widget>(), refresh.rightColumnID);
            refresh.assign_widget_to_node(rttr::type::get<player_widget>(), bottomLeftID); }));
}

auto root_widget::render_menu_implementation() -> void
{
    static bool showMenu  = false;
    static bool showAbout = false;

    gluten::imgui::scoped_font fontLucide(get_app()->get_font(gluten::fonts::regular_lucide_icons));

    {
        if (ImGui::BeginMenu(s_fileMenuName))
        {
            if (ImGui::MenuItem(ICON_LC_FILE " New...", "Ctrl+N"))
            {
                get_app()->get_manager_by_class<app_manager>()->create_new_project();
            }

            if (ImGui::MenuItem(ICON_LC_FOLDER_OPEN " Open...", "Ctrl+O"))
            {
                get_app()->get_manager_by_class<app_manager>()->open_project();
            }

            if (get_app()->get_manager_by_class<project_manager>())
            {
                ImGui::Separator();

                if (ImGui::MenuItem(ICON_LC_SAVE " Save", "Ctrl+S"))
                {
                    get_app()->get_manager_by_class<project_manager>()->save_project();
                }

                if (ImGui::MenuItem(ICON_LC_SAVE_ALL " Save As...", "Shift+Ctrl+S"))
                {
                }

                ImGui::Separator();
                if (ImGui::MenuItem(ICON_LC_HAMMER " Convert Files", nullptr, nullptr))
                {
                    sbk::engine::system::get()->get_project()->encode_all_media();
                }

                if (ImGui::MenuItem(ICON_LC_WRENCH " Build Soundbanks", nullptr, nullptr))
                {
                    sbk::engine::system::get()->get_project()->build_soundbanks();
                }
            }

            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4"))
            {
                get_app()->request_exit();
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu(s_editMenuName))
        {
            if (get_app()->get_manager_by_class<project_manager>())
            {
                ImGui::BeginDisabled(true);
                if (ImGui::MenuItem(ICON_LC_UNDO " Undo", "Ctrl+Z"))
                {
                }

                if (ImGui::MenuItem(ICON_LC_REDO " Redo", "Ctrl+Shift+Z"))
                {
                }
                ImGui::EndDisabled();
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu(s_helpMenuName))
        {
            if (ImGui::MenuItem(ICON_LC_BOOK_OPEN_TEXT " View Documentation"))
            {
                ImGuiContext* const context = ImGui::GetCurrentContext();
                if (context->PlatformIO.Platform_OpenInShellFn != NULL)
                {
                    context->PlatformIO.Platform_OpenInShellFn(context, "https://soundbakery.jameskelly.audio");
                }
            }

            ImGui::Separator();
            if (ImGui::MenuItem(ICON_LC_BOOK_HEART " About " SBK_PRODUCT_NAME "..."))
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

    gluten::root_widget::render_menu_implementation();
}

void root_widget::render_about_window(bool& showAbout)
{
    if (ImGui::Begin("About " SBK_PRODUCT_NAME, &showAbout, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("%s %s", SBK_PRODUCT_NAME, SBK_VERSION_STRING);

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
