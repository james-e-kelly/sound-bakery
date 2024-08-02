#include "root_widget.h"

#include "gluten/elements/text.h"
#include "gluten/elements/layouts/layout.h"
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

        gluten::imgui::scoped_color clearHeaderColor(ImGuiCol_WindowBg, gluten::theme::missingMesh);

            ImGuiStyle& style                 = ImGui::GetStyle();
        style.Colors[ImGuiCol_WindowBg].w = 0.0f;

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

    const ImVec2 windowStart = ImGui::GetWindowPos();
    const ImVec2 windowSize  = ImGui::GetWindowSize();
    const ImVec2 windowEnd   = ImVec2(windowStart.x + windowSize.x, windowStart.y + windowSize.y);

    const ImRect windowParent{windowStart, ImVec2(windowEnd.x, windowStart.y + titlebarHeight)};
    
    //gluten::element::s_debug = true;

    gluten::layout topBarLayout(gluten::layout::layout_type::left_to_right);
    topBarLayout.get_element_anchor().set_achor_from_preset(gluten::element::anchor_preset::stretch_full);
    topBarLayout.set_element_background_color(gluten::theme::titlebar);
    topBarLayout.render(windowParent);

    gluten::layout logoLayout(gluten::layout::layout_type::left_to_right);
    logoLayout.get_element_anchor().set_achor_from_preset(gluten::element::anchor_preset::stretch_full);
    topBarLayout.render_layout_element_pixels_horizontal(&logoLayout, 64.0f);
    logoLayout.render_layout_element_percent(get_app()->get_window_icon(), 1.0f, 1.0f);

    gluten::layout centerTitleLayout(gluten::layout::layout_type::left_to_right);
    centerTitleLayout.get_element_anchor().set_achor_from_preset(gluten::element::anchor_preset::center_middle);
    centerTitleLayout.render(windowParent);

    gluten::text titleText(std::string(get_app()->get_application_display_title()));
    titleText.set_element_alignment(ImVec2(0.5f, 0.5f));
    centerTitleLayout.render_layout_element_percent(&titleText, 1.0f, 1.0f);

    const float w                = ImGui::GetContentRegionAvail().x;
    const float buttonsAreaWidth = 94;

    
    ImGui::SetCursorPos(ImVec2(windowPadding.x, windowPadding.y + titlebarVerticalOffset));  // Reset cursor pos
    ImGui::InvisibleButton("##titleBarDragZone", ImVec2(w - buttonsAreaWidth, titlebarHeight));


    m_hoveringTitlebar = ImGui::IsItemHovered();

    ImGui::SetCursorPosY(titlebarHeight);

    //if (isMaximized)
    //{
    //    const float MouseY = ImGui::GetMousePos().y;
    //    const float DrawMouseY = ImGui::GetCursorScreenPos().y;
    //    const float windowMousePosY = MouseY - DrawMouseY;

    //    if (windowMousePosY >= 0.0f && windowMousePosY <= 5.0f)
    //    {
    //        m_hoveringTitlebar = true;  // Account for the top-most pixels which don't register
    //    }
    //}

    //// Draw Menubar
    //{
    //    ImGui::SetItemAllowOverlap();
    //    const float logoHorizontalOffset = 16.0f * 2.0f + 48.0f + windowPadding.x;
    //    ImGui::SetCursorPos(ImVec2(logoHorizontalOffset, 6.0f + titlebarVerticalOffset));
    //    //render_menu();

    //    if (ImGui::IsItemHovered())
    //    {
    //        //m_hoveringTitlebar = false;
    //    }
    //}
    //
    //{
    //    gluten::text windowTitleText(std::string(get_app()->get_application_display_title()));

    //}
    //
    //// Minimize Button
    //{
    //    if (get_app()->get_window_minimise_icon()->render())
    //    {
    //        /*if (m_WindowHandle)
    //        {
    //            Application::Get().QueueEvent([windowHandle = m_WindowHandle]() { glfwIconifyWindow(windowHandle); });
    //        }*/
    //    }
    //}

    //// Maximize Button
    //{
    //    if (const bool isMaximized = get_app()->is_maximized())
    //    {
    //        if (get_app()->get_window_restore_icon()->render())
    //        {
    //            get_app()->get_subsystem_by_class<gluten::renderer_subsystem>()->toggle_maximised();
    //        }
    //    }
    //    else
    //    {
    //        if (get_app()->get_window_maximise_icon()->render())
    //        {
    //            get_app()->get_subsystem_by_class<gluten::renderer_subsystem>()->toggle_maximised();
    //        }
    //    }
    //}

    //// Close Button
    //{
    //    if (get_app()->get_window_close_icon()->render())
    //    {
    //        get_app()->request_exit();
    //    }
    //}

    //ImGui::SetCursorPosY(titlebarHeight);
}