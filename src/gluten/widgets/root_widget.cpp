#include "root_widget.h"

#include "gluten/subsystems/renderer_subsystem.h"
#include "gluten/utils/imgui_util_functions.h"
#include "gluten/utils/imgui_util_structures.h"
#include "gluten/theme/theme.h"

#include "IconsFontAwesome6.h"
#include "IconsFontaudio.h"
#include "app/app.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_stacklayout.h"

using namespace gluten;

static const ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_None;
static const ImGuiWindowFlags rootWindowFlags  = ImGuiWindowFlags_NoDocking |
                                          ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                          ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                          ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

static const char* rootWindowName = "RootDockSpace"; 

void root_widget::render()
{
    set_root_window_to_viewport();   

    {
        gluten::imgui::scoped_style_stack rootStyle(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f),
                                                    ImGuiStyleVar_WindowRounding, 0.0f,
                                                    ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::Begin(rootWindowName, nullptr, rootWindowFlags);
    }

    draw_titlebar();
    submit_dockspace();

    {
        gluten::imgui::scoped_font font(ImGui::GetIO().Fonts->Fonts[1]);

        const ImRect menuBarRect = {
            ImGui::GetCursorPos(),
            {ImGui::GetContentRegionAvail().x + ImGui::GetCursorScreenPos().x, ImGui::GetFrameHeightWithSpacing()}};

        /*ImGui::BeginGroup();
        if (gluten::imgui::begin_menubar(menuBarRect))
        {
            render_menu();
        }

        gluten::imgui::end_menubar();*/
    }

    render_children();

    ImGui::End();
}

void gluten::root_widget::submit_dockspace()
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspaceId = ImGui::GetID(rootWindowName);
        ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), dockspaceFlags);
    }
}

void root_widget::set_root_window_to_viewport()
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
}

void root_widget::draw_titlebar()
{
    const float titlebarHeight   = 58.0f;
    const bool isMaximized       = get_app()->is_maximized();
    float titlebarVerticalOffset = isMaximized ? -6.0f : 0.0f;
    const ImVec2 windowPadding   = ImGui::GetCurrentWindow()->WindowPadding;

    ImGui::SetCursorPos(ImVec2(windowPadding.x, windowPadding.y + titlebarVerticalOffset));
    const ImVec2 titlebarMin = ImGui::GetCursorScreenPos();
    const ImVec2 titlebarMax = {ImGui::GetCursorScreenPos().x + ImGui::GetWindowWidth(), ImGui::GetCursorScreenPos().y + titlebarHeight};
    auto* bgDrawList         = ImGui::GetBackgroundDrawList();
    auto* fgDrawList = ImGui::GetForegroundDrawList();
    bgDrawList->AddRectFilled(titlebarMin, titlebarMax, gluten::theme::titlebar);

    // DEBUG TITLEBAR BOUNDS
    //fgDrawList->AddRect(titlebarMin, titlebarMax, gluten::theme::invalidPrefab);


    ImGui::BeginHorizontal("Titlebar",
                           {ImGui::GetWindowWidth() - windowPadding.y * 2.0f, ImGui::GetFrameHeightWithSpacing()});
    // Logo
    {
        get_app()->get_window_icon()->render();
    }

    const float w                = ImGui::GetContentRegionAvail().x;
    const float buttonsAreaWidth = 94;

    // Title bar drag area
    // On Windows we hook into the GLFW win32 window internals
    ImGui::SetCursorPos(ImVec2(windowPadding.x, windowPadding.y + titlebarVerticalOffset));  // Reset cursor pos
    // DEBUG DRAG BOUNDS
     fgDrawList->AddRect(ImGui::GetCursorScreenPos(), ImVec2(ImGui::GetCursorScreenPos().x + w - buttonsAreaWidth,
     ImGui::GetCursorScreenPos().y + titlebarHeight), gluten::theme::invalidPrefab);
    ImGui::InvisibleButton("##titleBarDragZone", ImVec2(w - buttonsAreaWidth, titlebarHeight));

    m_hoveringTitlebar = ImGui::IsItemHovered();

    if (isMaximized)
    {
        const float MouseY = ImGui::GetMousePos().y;
        const float DrawMouseY = ImGui::GetCursorScreenPos().y;
        const float windowMousePosY = MouseY - DrawMouseY;

        if (windowMousePosY >= 0.0f && windowMousePosY <= 5.0f)
            m_hoveringTitlebar = true;  // Account for the top-most pixels which don't register
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
            //m_hoveringTitlebar = false;
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
    
    // Minimize Button
    ImGui::Spring();
    gluten::imgui::shift_cursor_y(8.0f);
    {
        if (get_app()->get_window_minimise_icon()->button("Minimise"))
        {
            /*if (m_WindowHandle)
            {
                Application::Get().QueueEvent([windowHandle = m_WindowHandle]() { glfwIconifyWindow(windowHandle); });
            }*/
        }
    }

    // Maximize Button
    ImGui::Spring(-1.0f, 17.0f);
    gluten::imgui::shift_cursor_y(8.0f);
    {
        if (const bool isMaximized = get_app()->is_maximized())
        {
            if (get_app()->get_window_restore_icon()->button("Restore"))
            {
                get_app()->get_subsystem_by_class<gluten::renderer_subsystem>()->toggle_maximised();
            }
        }
        else
        {
            if (get_app()->get_window_maximise_icon()->button("Maximise"))
            {
                get_app()->get_subsystem_by_class<gluten::renderer_subsystem>()->toggle_maximised();
            }
        }
    }

    // Close Button
    ImGui::Spring(-1.0f, 15.0f);
    gluten::imgui::shift_cursor_y(8.0f);
    {
        if (get_app()->get_window_close_icon()->button("Close"))
        {
            get_app()->request_exit();
        }
    }

    ImGui::Spring(-1.0f, 18.0f);
    ImGui::EndHorizontal();

    ImGui::SetCursorPosY(titlebarHeight);
}