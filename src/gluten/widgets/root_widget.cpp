#include "root_widget.h"

#include "gluten/elements/text.h"
#include "gluten/elements/menu_bar.h"
#include "gluten/elements/layouts/layout.h"
#include "gluten/subsystems/renderer_subsystem.h"
#include "gluten/utils/imgui_util_functions.h"
#include "gluten/utils/imgui_util_structures.h"
#include "gluten/theme/theme.h"
#include "gluten/theme/carbon_theme_g100.h"
#include "gluten/theme/window_images.embed"
#include "gluten/theme/walnut_icon.embed"

#include "IconsFontAwesome6.h"
#include "IconsFontaudio.h"
#include "app/app.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "implot.h"

#include <cmrc/cmrc.hpp>

CMRC_DECLARE(sbk::icon_images);

using namespace gluten;

static const ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_None;
static const ImGuiWindowFlags rootWindowFlags  = ImGuiWindowFlags_NoDocking |
                                          ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                          ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                          ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

static const char* rootWindowName = "RootDockSpace"; 

namespace root_widget_utils
{
    constexpr float fontSize = 20.0f;
    constexpr float titleFontSize = 22.0f;

    static float titleBarHeight()
    {
        ImGuiContext* const context = ImGui::GetCurrentContext();
        return context == nullptr ? 0.0f : context->FontSize * gluten::theme::carbon_g100::appTitlebarHeightMultiplier;
    }

    constexpr float titleLogoWidth = 64.0f;
    constexpr float menuBarPaddingWithLogo   = 6.0f;
    constexpr float menuBarStartWidth = titleLogoWidth + 6.0f;
    constexpr float titleBarButtonsAreaWidth = 256.0f;
    constexpr float maximisedYOffset         = 6.0f;
    static float menuBarYOffset           = titleBarHeight() / 8.0f;

    static ImRect get_titlebar_rect(const ImGuiWindow* const window)
    {
        ImRect windowRect = window->Rect();
        windowRect.Add(window->Pos);
        windowRect.Max.y = windowRect.Min.y + root_widget_utils::titleBarHeight();

        if (gluten::app::get()->is_maximized())
        {
            windowRect.TranslateY(maximisedYOffset);
        }

        return windowRect;
    }

    static ImRect get_logo_rect(const ImRect& titlebarRect)
    {
        ImRect logoRect = titlebarRect;
        logoRect.Max.x = logoRect.Min.x + titleLogoWidth;
        return logoRect;
    }

    static ImRect get_menu_bar_rect(const ImRect& titlebarRect)
    { 
        ImRect menuBarRect = titlebarRect;
        menuBarRect.Min.x += titleLogoWidth + menuBarPaddingWithLogo;
        menuBarRect.TranslateY(menuBarYOffset);
        return menuBarRect;
    }

    static ImRect get_drag_zone_rect(const ImRect& titlebarRect)
    { 
        ImRect dragZoneRect = titlebarRect;
        dragZoneRect.Max.x -= titleBarButtonsAreaWidth;
        return dragZoneRect;
    }

    static ImRect get_buttons_rect(const ImRect& titlebarRect)
    { 
        ImRect buttonRect = titlebarRect;
        buttonRect.Min.x = buttonRect.Max.x - titleBarButtonsAreaWidth;
        return buttonRect;
    }

    static ImRect get_side_bar_rect(const ImRect& titlebarRect)
    {
        ImRect sideBarRect = titlebarRect;
        sideBarRect.Max.x  = sideBarRect.Min.x + titleBarHeight() + menuBarPaddingWithLogo;
        sideBarRect.Min.y  = titlebarRect.Max.y;
        sideBarRect.Max.y  = titlebarRect.Max.y + ImGui::GetWindowHeight();
        return sideBarRect;
    }
}

static bool showUserGuide = false;

void root_widget::start()
{ 
    widget::start();

    const cmrc::embedded_filesystem embeddedfilesystem = cmrc::sbk::icon_images::get_filesystem();
    const cmrc::file logoFile = embeddedfilesystem.open("sound-bakery-logo.png");
    assert(logoFile.size() > 0);

    m_windowIcon      = std::make_unique<gluten::image>(logoFile.cbegin(), logoFile.size());
    m_windowCloseIcon = std::make_unique<gluten::image_button>("Close", g_WindowCloseIcon, sizeof(g_WindowCloseIcon));
    m_windowMinimiseIcon = std::make_unique<gluten::image_button>("Minimise", g_WindowMinimiseIcon, sizeof(g_WindowMinimiseIcon));
    m_windowMaximiseIcon = std::make_unique<gluten::image_button>("Maximise", g_WindowMaximiseIcon, sizeof(g_WindowMaximiseIcon));
    m_windowRestoreIcon = std::make_unique<gluten::image_button>("Restore", g_WindowRestoreIcon, sizeof(g_WindowRestoreIcon));

    m_windowIcon->get_element_anchor().set_achor_from_preset(gluten::element::anchor_preset::stretch_full);
    m_windowCloseIcon->get_element_anchor().set_achor_from_preset(gluten::element::anchor_preset::stretch_full);
    m_windowMinimiseIcon->get_element_anchor().set_achor_from_preset(gluten::element::anchor_preset::stretch_full);
    m_windowMaximiseIcon->get_element_anchor().set_achor_from_preset(gluten::element::anchor_preset::stretch_full);
    m_windowRestoreIcon->get_element_anchor().set_achor_from_preset(gluten::element::anchor_preset::stretch_full);

    m_windowIcon->set_element_max_size(ImVec2(root_widget_utils::titleBarHeight(), root_widget_utils::titleBarHeight()));
    m_windowCloseIcon->set_element_max_size(ImVec2(16, 16));
    m_windowMinimiseIcon->set_element_max_size(ImVec2(14, 14));
    m_windowMaximiseIcon->set_element_max_size(ImVec2(14, 14));
    m_windowRestoreIcon->set_element_max_size(ImVec2(16, 16));

    m_windowCloseIcon->set_element_hover_color(
        theme::ColorWithMultipliedValue(ImGui::ColorConvertFloat4ToU32(gluten::theme::carbon_g100::background), 2.f));
    m_windowMinimiseIcon->set_element_hover_color(
        theme::ColorWithMultipliedValue(ImGui::ColorConvertFloat4ToU32(gluten::theme::carbon_g100::background), 2.f));
    m_windowMaximiseIcon->set_element_hover_color(
        theme::ColorWithMultipliedValue(ImGui::ColorConvertFloat4ToU32(gluten::theme::carbon_g100::background), 2.f));
    m_windowRestoreIcon->set_element_hover_color(
        theme::ColorWithMultipliedValue(ImGui::ColorConvertFloat4ToU32(gluten::theme::carbon_g100::background), 2.f));
}

void root_widget::render()
{
    set_root_window_to_viewport();   

    {
        gluten::imgui::scoped_style_stack rootStyle(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f),
                                                    ImGuiStyleVar_WindowRounding, 0.0f, 
                                                    ImGuiStyleVar_WindowBorderSize, 0.0f);

        gluten::imgui::scoped_color clearHeaderColor(ImGuiCol_WindowBg, gluten::theme::invalidPrefab);

            ImGuiStyle& style                 = ImGui::GetStyle();
        style.Colors[ImGuiCol_WindowBg].w = 0.0f;

        ImGui::Begin(rootWindowName, nullptr, rootWindowFlags);
    }

    draw_titlebar();
    submit_dockspace();

    if (showUserGuide)
    {
        if (ImGui::Begin("User Guide", &showUserGuide))
        {
            ImGui::ShowUserGuide();
        }
        ImGui::End();
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
    const ImRect titleBarRect = root_widget_utils::get_titlebar_rect(ImGui::GetCurrentWindow());
    const ImRect logoRect     = root_widget_utils::get_logo_rect(titleBarRect);
    const ImRect dragZoneRect = root_widget_utils::get_drag_zone_rect(titleBarRect);
    const ImRect buttonsRect  = root_widget_utils::get_buttons_rect(titleBarRect);
    const ImRect menuBarRect  = root_widget_utils::get_menu_bar_rect(titleBarRect);

    element topBarBackground(element::anchor_preset::stretch_full);
    topBarBackground.set_element_background_color(ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_TitleBg)));
    topBarBackground.render(titleBarRect);

    m_windowIcon->set_element_frame_padding();
    m_windowIcon->render(logoRect);

    gluten::text titleText(std::string(get_app()->get_application_display_title()), ImVec2(0.5f, 0.5f), element::anchor_preset::center_middle);
    titleText.set_font_size(root_widget_utils::titleFontSize);
    titleText.set_font(gluten::fonts::regular);
    titleText.render(titleBarRect);

    gluten::button dragZoneButton("##titleBarDragZone", true);
    dragZoneButton.get_element_anchor().set_achor_from_preset(gluten::element::anchor_preset::stretch_full);
    dragZoneButton.render(dragZoneRect);

    m_hoveringTitlebar = ImGui::IsItemHovered();

    gluten::menu_bar menu_bar;
    menu_bar.set_font_size(root_widget_utils::fontSize);
    if (menu_bar.render(menuBarRect))
    {
        render_menu();
    }
    menu_bar.end_menu_bar();

    gluten::layout titleButtonsLayout(layout::layout_type::right_to_left);
    titleButtonsLayout.get_element_anchor().set_achor_from_preset(element::anchor_preset::stretch_full);
    titleButtonsLayout.reset_layout(buttonsRect);

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

    ImGui::SetCursorScreenPos(titleBarRect.GetBL());
}

void root_widget::render_menu()
{
    static bool showMetricsWindow = false;
    static bool showDebugLogWindow = false;
    static bool showStackTool = false;
    static bool showAboutWindow    = false;
    static bool showStyleEditor    = false;
    static bool showDemo = false;
    static bool showPlotDemo = false;
      
    if (ImGui::BeginMenu("Help"))
    {
        ImGui::Separator();

        if (ImGui::BeginMenu("UI"))
        {
            ImGui::MenuItem("ImGui About...", NULL, &showAboutWindow);
            ImGui::MenuItem("ImGui User Guide...", NULL, &showUserGuide);
            ImGui::MenuItem("ImGui Metrics...", NULL, &showMetricsWindow);
            ImGui::MenuItem("ImGui Debug...", NULL, &showDebugLogWindow);
            ImGui::MenuItem("ImGui Stack Tool...", NULL, &showStackTool);
            ImGui::MenuItem("ImGui Style Editor...", NULL, &showStyleEditor);
            ImGui::MenuItem("ImGui Demo...", NULL, &showDemo);
            ImGui::MenuItem("ImPlot Demo...", NULL, &showPlotDemo);

            ImGui::Separator();

            ImGui::MenuItem("Debug Item Rects", NULL, &element::s_debug);

            ImGui::EndMenu();
        }
        ImGui::EndMenu();
    }

    if (showMetricsWindow)
    {
        ImGui::ShowMetricsWindow(&showMetricsWindow);
    }
    if (showDebugLogWindow)
    {
        ImGui::ShowDebugLogWindow(&showDebugLogWindow);
    }
    if (showStackTool)
    {
        ImGui::ShowIDStackToolWindow(&showStackTool);
    }
    if (showAboutWindow)
    {
        ImGui::ShowAboutWindow(&showAboutWindow);
    }
    if (showStyleEditor)
    {
        if (ImGui::Begin("Style Editor", &showStyleEditor))
        {
            ImGui::ShowStyleEditor();
        }
        ImGui::End();
    }
    if (showDemo)
    {
        ImGui::ShowDemoWindow(&showDemo);
    }
    if (showPlotDemo)
    {
        ImPlot::ShowDemoWindow(&showPlotDemo);
    }
}