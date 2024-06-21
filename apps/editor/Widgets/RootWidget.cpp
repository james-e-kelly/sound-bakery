#include "RootWidget.h"

#include "App/App.h"
#include "Managers/AppManager.h"
#include "Managers/ProjectManager.h"
#include "IconsFontAwesome6.h"
#include "IconsFontaudio.h"
#include "imgui.h"

#include "sound_bakery/editor/project/project.h"

void root_widget::render()
{
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

    // We are using the ImGuiWindowFlags_NoDocking flag to make the parent
    // window not dockable into, because it would be confusing to have two
    // docking targets within each others.
    ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |=
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will
    // render our background and handle the pass-thru hole, so we ask Begin() to
    // not render a background.
    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace Demo", nullptr, window_flags);
    ImGui::PopStyleVar();

    ImGui::PopStyleVar(2);

    // Submit the DockSpace
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID("RootDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }

    static bool showMenu = false;
    bool showAbout       = false;

    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {

            if (ImGui::MenuItem(ICON_FA_FILE " New...", "Ctrl+N"))
            {
                get_app()->get_manager_by_class<AppManager>()->CreateNewProject();
            }

            if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN " Open...", "Ctrl+O"))
            {
                get_app()->get_manager_by_class<AppManager>()->OpenProject();
            }

            if (get_app()->get_manager_by_class<ProjectManager>())
            {
                ImGui::Separator();

                ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
                if (ImGui::MenuItem(ICON_FAD_SAVE " Save", "Ctrl+S"))
                {
                    get_app()->get_manager_by_class<ProjectManager>()->SaveProject();
                }

                if (ImGui::MenuItem(ICON_FAD_SAVEAS " Save As...",
                                    "Shift+Ctrl+S"))
                {
                }
                ImGui::PopFont();
            }

            ImGui::Separator();
            if (ImGui::MenuItem("exit", "Alt+F4"))
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
                sbk::engine::system::get_project()->encodeAllMedia();
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

        ImGui::EndMenuBar();
    }

    ImGui::PopFont();

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

    render_children();

    ImGui::End();
}