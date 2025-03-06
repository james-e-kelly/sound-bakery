#include "root_widget.h"

#include "app/app.h"
#include "IconsFontAwesome6.h"
#include "IconsFontaudio.h"
#include "imgui.h"

void root_widget::render_menu_implementation()
{
    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem(ICON_FA_FILE " Open Soundbank..."))
        {
            static_cast<game_app*>(get_app())->open_soundbank();
        }

        ImGui::Separator();
        if (ImGui::MenuItem("Exit", "Alt+F4"))
        {
            get_app()->request_exit();
        }

        ImGui::EndMenu();
    }

    gluten::root_widget::render_menu();
}