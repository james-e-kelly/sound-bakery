#include "root_widget.h"

#include "gluten/elements/text.h"
#include "gluten/elements/layouts/layout.h"
#include "gluten/subsystems/renderer_subsystem.h"
#include "gluten/utils/imgui_util_functions.h"
#include "gluten/utils/imgui_util_structures.h"
#include "gluten/theme/theme.h"
#include "gluten/theme/window_images.embed"
#include "gluten/theme/walnut_icon.embed"

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

void root_widget::start()
{ 
    widget::start();

    m_windowIcon      = std::make_unique<gluten::image>(g_WalnutIcon, sizeof(g_WalnutIcon));
    m_windowCloseIcon = std::make_unique<gluten::image_button>("Close", g_WindowCloseIcon, sizeof(g_WindowCloseIcon));
    m_windowMinimiseIcon = std::make_unique<gluten::image_button>("Minimise", g_WindowMinimiseIcon, sizeof(g_WindowMinimiseIcon));
    m_windowMaximiseIcon = std::make_unique<gluten::image_button>("Maximise", g_WindowMaximiseIcon, sizeof(g_WindowMaximiseIcon));
    m_windowRestoreIcon = std::make_unique<gluten::image_button>("Restore", g_WindowRestoreIcon, sizeof(g_WindowRestoreIcon));

    m_windowIcon->get_element_anchor().set_achor_from_preset(gluten::element::anchor_preset::stretch_full);
    m_windowCloseIcon->get_element_anchor().set_achor_from_preset(gluten::element::anchor_preset::stretch_full);
    m_windowMinimiseIcon->get_element_anchor().set_achor_from_preset(gluten::element::anchor_preset::stretch_full);
    m_windowMaximiseIcon->get_element_anchor().set_achor_from_preset(gluten::element::anchor_preset::stretch_full);
    m_windowRestoreIcon->get_element_anchor().set_achor_from_preset(gluten::element::anchor_preset::stretch_full);

    m_windowCloseIcon->set_element_max_size(ImVec2(16, 16));
    m_windowMinimiseIcon->set_element_max_size(ImVec2(14, 14));
    m_windowMaximiseIcon->set_element_max_size(ImVec2(14, 14));
    m_windowRestoreIcon->set_element_max_size(ImVec2(16, 16));

    m_windowCloseIcon->set_element_hover_color(theme::ColorWithMultipliedValue(gluten::theme::titlebar, 2.f));
    m_windowMinimiseIcon->set_element_hover_color(theme::ColorWithMultipliedValue(gluten::theme::titlebar, 2.f));
    m_windowMaximiseIcon->set_element_hover_color(theme::ColorWithMultipliedValue(gluten::theme::titlebar, 2.f));
    m_windowRestoreIcon->set_element_hover_color(theme::ColorWithMultipliedValue(gluten::theme::titlebar, 2.f));
}

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
        gluten::imgui::scoped_font font(get_app()->get_font(fonts::regular));

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
    const float titlebarHeight   = 48.0f;
    const bool isMaximized       = get_app()->is_maximized();
    const float titlebarVerticalOffset = isMaximized ? 6.0f : 0.0f;

    const ImVec2 windowPos = ImGui::GetWindowPos();
    const ImVec2 windowStart = ImVec2(windowPos.x, windowPos.y + titlebarVerticalOffset);
    const ImVec2 windowSize  = ImGui::GetWindowSize();
    const ImVec2 windowEnd   = ImVec2(windowStart.x + windowSize.x, windowStart.y + windowSize.y);

    const ImRect windowParent{windowStart, ImVec2(windowEnd.x, windowStart.y + titlebarHeight + titlebarVerticalOffset)};

    if (isMaximized)
    {
        assert(windowParent.GetSize().y > 50.0f);
    }
    
    //gluten::element::s_debug = true;

    gluten::layout topBarLayout(gluten::layout::layout_type::left_to_right);
    topBarLayout.get_element_anchor().set_achor_from_preset(gluten::element::anchor_preset::stretch_full);
    topBarLayout.set_element_background_color(gluten::theme::titlebar);
    topBarLayout.render(windowParent);  // render background

    gluten::layout logoLayout(gluten::layout::layout_type::left_to_right);
    logoLayout.get_element_anchor().set_achor_from_preset(gluten::element::anchor_preset::stretch_full);
    topBarLayout.render_layout_element_pixels_horizontal(&logoLayout, 64.0f);
    logoLayout.render_layout_element_percent(m_windowIcon.get(), 1.0f, 1.0f);

    {
        gluten::imgui::scoped_font titleFont(get_app()->get_font(fonts::title));
        gluten::text titleText(std::string(get_app()->get_application_display_title()));
        titleText.set_element_alignment(ImVec2(0.5f, 0.5f));
        titleText.get_element_anchor().set_achor_from_preset(element::anchor_preset::center_middle);
        titleText.render(windowParent);
    }

    const float titleButtonsAreaWidth = 256.0f;
    const float titleBarStartX        = windowStart.x;
    const float titleBarButtonStartX  = windowEnd.x - titleButtonsAreaWidth;

    const ImRect titleBarAreaLeftOfButtons = ImRect(windowParent.Min, ImVec2(titleBarButtonStartX, windowParent.Max.y));
    const ImRect titleBarAreaButtons       = ImRect(ImVec2(titleBarButtonStartX, windowStart.y), windowParent.Max);

    gluten::button dragZoneButton("##titleBarDragZone", true);
    dragZoneButton.get_element_anchor().set_achor_from_preset(gluten::element::anchor_preset::stretch_full);
    dragZoneButton.render(titleBarAreaLeftOfButtons);

    m_hoveringTitlebar = ImGui::IsItemHovered();

    gluten::layout titleButtonsLayout(layout::layout_type::right_to_left);
    titleButtonsLayout.get_element_anchor().set_achor_from_preset(element::anchor_preset::stretch_full);
    titleButtonsLayout.reset_layout(titleBarAreaButtons);

    if (titleButtonsLayout.render_layout_element_percent_horizontal(m_windowCloseIcon.get(), 0.33f))
    {
        get_app()->request_exit();
    }

    if (const bool isMaximized = get_app()->is_maximized())
    {
        if (titleButtonsLayout.render_layout_element_percent_horizontal(m_windowRestoreIcon.get(), 0.33f))
        {
            get_app()->get_subsystem_by_class<gluten::renderer_subsystem>()->toggle_maximised();
        }
    }
    else
    {
        if (titleButtonsLayout.render_layout_element_percent_horizontal(m_windowMaximiseIcon.get(), 0.33f))
        {
            get_app()->get_subsystem_by_class<gluten::renderer_subsystem>()->toggle_maximised();
        }
    }

    if (titleButtonsLayout.render_layout_element_percent_horizontal(m_windowMinimiseIcon.get(), 0.33f))
    {
        get_app()->get_subsystem_by_class<gluten::renderer_subsystem>()->toggle_minimised();
    }

    ImGui::SetCursorScreenPos(ImVec2(windowParent.Min.x, windowParent.Max.y));

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
}