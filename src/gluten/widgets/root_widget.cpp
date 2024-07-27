#include "root_widget.h"

#include "gluten/subsystems/renderer_subsystem.h"
#include "gluten/utils/imgui_util_functions.h"
#include "gluten/theme/theme.h"

#include "IconsFontAwesome6.h"
#include "IconsFontaudio.h"
#include "app/app.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_stacklayout.h"

using namespace gluten;

void root_widget::render()
{
    static ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_None;

    // We are using the ImGuiWindowFlags_NoDocking flag to make the get_parent
    // window not dockable into, because it would be confusing to have two
    // docking targets within each others.
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    window_flags |=
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_MenuBar;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will
    // render our background and handle the pass-thru hole, so we ask Begin() to
    // not render a background.
    if (dockspaceFlags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("RootDockSpace", nullptr, window_flags);
    ImGui::PopStyleVar();

    ImGui::PopStyleVar(2);

    float titleBarHeight;
    draw_titlebar(titleBarHeight);
    ImGui::SetCursorPosY(titleBarHeight);

    // Submit the DockSpace
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID("RootDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspaceFlags);
    }

    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);

    const ImRect menuBarRect = {ImGui::GetCursorPos(), {ImGui::GetContentRegionAvail().x + ImGui::GetCursorScreenPos().x, ImGui::GetFrameHeightWithSpacing()}};

    /*ImGui::BeginGroup();
    if (gluten::imgui::begin_menubar(menuBarRect))
    {
        render_menu();
    }

    gluten::imgui::end_menubar();*/

    ImGui::PopFont();

    render_children();

    ImGui::End();
}

void root_widget::draw_titlebar(float& outTitlebarHeight)
{
    const float titlebarHeight   = 58.0f;
    const bool isMaximized       = get_app()->is_maximized();
    float titlebarVerticalOffset = isMaximized ? -6.0f : 0.0f;
    const ImVec2 windowPadding   = ImGui::GetCurrentWindow()->WindowPadding;

    ImGui::SetCursorPos(ImVec2(windowPadding.x, windowPadding.y + titlebarVerticalOffset));
    const ImVec2 titlebarMin = ImGui::GetCursorScreenPos();
    const ImVec2 titlebarMax = {ImGui::GetCursorScreenPos().x + ImGui::GetWindowWidth() - windowPadding.y * 2.0f,
                                ImGui::GetCursorScreenPos().y + titlebarHeight};
    auto* bgDrawList         = ImGui::GetBackgroundDrawList();
    auto* fgDrawList         = ImGui::GetForegroundDrawList();
    bgDrawList->AddRectFilled(titlebarMin, titlebarMax, gluten::theme::titlebar);
    // DEBUG TITLEBAR BOUNDS
    // fgDrawList->AddRect(titlebarMin, titlebarMax, UI::Colors::Theme::invalidPrefab);

    // Logo
    {
        const int logoWidth  = 48;  // m_LogoTex->GetWidth();
        const int logoHeight = 48;  // m_LogoTex->GetHeight();
        const ImVec2 logoOffset(16.0f + windowPadding.x, 5.0f + windowPadding.y + titlebarVerticalOffset);
        const ImVec2 logoRectStart = {ImGui::GetItemRectMin().x + logoOffset.x,
                                      ImGui::GetItemRectMin().y + logoOffset.y};
        const ImVec2 logoRectMax   = {logoRectStart.x + logoWidth, logoRectStart.y + logoHeight};
        //fgDrawList->AddImage(m_AppHeaderIcon->GetDescriptorSet(), logoRectStart, logoRectMax);
    }

    ImGui::BeginHorizontal("Titlebar",
                           {ImGui::GetWindowWidth() - windowPadding.y * 2.0f, ImGui::GetFrameHeightWithSpacing()});

    static float moveOffsetX;
    static float moveOffsetY;
    const float w                = ImGui::GetContentRegionAvail().x;
    const float buttonsAreaWidth = 94;

    // Title bar drag area
    // On Windows we hook into the GLFW win32 window internals
    ImGui::SetCursorPos(ImVec2(windowPadding.x, windowPadding.y + titlebarVerticalOffset));  // Reset cursor pos
    // DEBUG DRAG BOUNDS
    // fgDrawList->AddRect(ImGui::GetCursorScreenPos(), ImVec2(ImGui::GetCursorScreenPos().x + w - buttonsAreaWidth,
    // ImGui::GetCursorScreenPos().y + titlebarHeight), UI::Colors::Theme::invalidPrefab);
    ImGui::InvisibleButton("##titleBarDragZone", ImVec2(w - buttonsAreaWidth, titlebarHeight));

    static bool titleBarHovered = ImGui::IsItemHovered();

    if (isMaximized)
    {
        float windowMousePosY = ImGui::GetMousePos().y - ImGui::GetCursorScreenPos().y;
        if (windowMousePosY >= 0.0f && windowMousePosY <= 5.0f)
            titleBarHovered = true;  // Account for the top-most pixels which don't register
    }

    // Draw Menubar
    ImGui::SuspendLayout();
    {
        ImGui::SetItemAllowOverlap();
        const float logoHorizontalOffset = 16.0f * 2.0f + 48.0f + windowPadding.x;
        ImGui::SetCursorPos(ImVec2(logoHorizontalOffset, 6.0f + titlebarVerticalOffset));
        //render_menu();

        if (ImGui::IsItemHovered())
        {
            titleBarHovered = false;
        }
    }
    ImGui::ResumeLayout();
    

    {
        // Centered Window title
        ImVec2 currentCursorPos = ImGui::GetCursorPos();
        ImVec2 textSize         = ImGui::CalcTextSize(get_app()->get_application_display_title().data());
        ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() * 0.5f - textSize.x * 0.5f, 2.0f + windowPadding.y + 6.0f));
        ImGui::Text("%s", get_app()->get_application_display_title().data());  // Draw title
        ImGui::SetCursorPos(currentCursorPos);
    }

    // Window buttons
    const ImU32 buttonColN   = gluten::theme::ColorWithMultipliedValue(gluten::theme::text, 0.9f);
    const ImU32 buttonColH   = gluten::theme::ColorWithMultipliedValue(gluten::theme::text, 1.2f);
    const ImU32 buttonColP   = gluten::theme::textDarker;
    const float buttonWidth  = 14.0f;
    const float buttonHeight = 14.0f;

    // Minimize Button

    ImGui::Spring();
    gluten::imgui::shift_cursor_y(8.0f);
    {
        const int iconWidth  = get_app()->get_window_minimise_icon()->get_width();
        const int iconHeight = get_app()->get_window_minimise_icon()->get_width();
        const float padY     = (buttonHeight - (float)iconHeight) / 2.0f;
        if (ImGui::InvisibleButton("Minimize", ImVec2(buttonWidth, buttonHeight)))
        {
            // TODO: move this stuff to a better place, like Window class
            /*if (m_WindowHandle)
            {
                Application::Get().QueueEvent([windowHandle = m_WindowHandle]() { glfwIconifyWindow(windowHandle); });
            }*/
        }

        get_app()->get_window_minimise_icon()->render();
    }

    // Maximize Button
    ImGui::Spring(-1.0f, 17.0f);
    gluten::imgui::shift_cursor_y(8.0f);
    {
        const int iconWidth  = get_app()->get_window_maximise_icon()->get_width();
        const int iconHeight = get_app()->get_window_maximise_icon()->get_height();

        const bool isMaximized = get_app()->is_maximized();

        if (ImGui::InvisibleButton("Maximize", ImVec2(buttonWidth, buttonHeight)))
        {
            get_app()->get_subsystem_by_class<gluten::renderer_subsystem>()->toggle_maximised();
        }

        if (isMaximized)
        {
            get_app()->get_window_restore_icon()->render();
        }
        else
        {
            get_app()->get_window_maximise_icon()->render();
        }
    }

    // Close Button
    ImGui::Spring(-1.0f, 15.0f);
    gluten::imgui::shift_cursor_y(8.0f);
    {
        const int iconWidth  = get_app()->get_window_close_icon()->get_width();
        const int iconHeight = get_app()->get_window_close_icon()->get_height();
        if (ImGui::InvisibleButton("Close", ImVec2(buttonWidth, buttonHeight)))
        {
            get_app()->request_exit();
        }

        get_app()->get_window_close_icon()->render();
    }

    ImGui::Spring(-1.0f, 18.0f);
    ImGui::EndHorizontal();

    outTitlebarHeight = titlebarHeight;
}