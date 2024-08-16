#include "renderer_subsystem.h"

#include "gluten/subsystems/widget_subsystem.h"
#include "gluten/widgets/root_widget.h"
#include "gluten/theme/theme.h"
#include "gluten/theme/carbon_theme_g100.h"

#include "app/app.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <stdio.h>

#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h>  // Will drag system OpenGL headers

#define RGB_TO_FLOAT(rgb) (rgb) / 255.0F

namespace glslVersion
{
#if defined(__APPLE__)
    static const char* glslVersion = "#version 150";
#else
    static const char* glslVersion = "#version 130";
#endif
}  // namespace glslVersion

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

using namespace gluten;

renderer_subsystem::window_guard::window_guard(int width, int height, const std::string& windowName)
{
    m_window = glfwCreateWindow(width, height, windowName.c_str(), NULL, NULL);
    assert(m_window);

    GLFWwindow* currentContext = glfwGetCurrentContext();

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1);  // Enable vsync

    // Creating a window shouldn't auto steal the context
    if (currentContext != NULL)
    {
        glfwMakeContextCurrent(currentContext);
    }


    glfwSetWindowUserPointer(m_window, gluten::app::get());
    glfwSetTitlebarHitTestCallback(m_window,
        [](GLFWwindow* window, int x, int y, int* hit)
        {
            if (gluten::app* const app = (gluten::app*)glfwGetWindowUserPointer(window))
            {
                if (gluten::widget_subsystem* const widgetSubsystem = app->get_subsystem_by_class<gluten::widget_subsystem>())
                {
                    if (gluten::widget* const rootWidget = widgetSubsystem->get_root_widget())
                    {
                        if (gluten::root_widget* const rootWidgetCasted = (gluten::root_widget*)rootWidget)
                        {
                            *hit = rootWidgetCasted->is_hovering_titlebar();
                        }
                    }
                }
            }
        });

    ImGui_ImplGlfw_InitForOpenGL(m_window, true);

    // If ImGui backend is not created, make it
    if (ImGui::GetIO().BackendRendererUserData == nullptr)
    {
        ImGui_ImplOpenGL3_Init(glslVersion::glslVersion);
    }
}

renderer_subsystem::window_guard::window_guard(window_guard&& other) noexcept
{
    m_window       = other.m_window;
    other.m_window = nullptr;
}

renderer_subsystem::window_guard& renderer_subsystem::window_guard::operator=(window_guard&& other) noexcept
{
    if (this != &other)
    {
        m_window       = other.m_window;
        other.m_window = nullptr;
    }

    return *this;
}

void renderer_subsystem::window_guard::set_title(const std::string& title) 
{
    if (m_window && !title.empty())
    {
        glfwSetWindowTitle(m_window, title.c_str());
    }
}

renderer_subsystem::window_guard::~window_guard()
{
    if (m_window)
    {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
}

int renderer_subsystem::pre_init(int ArgC, char* ArgV[]) { return 0; }

void renderer_subsystem::set_default_window_hints()
{
    // Decide GL+GLSL versions
#if defined(__APPLE__)
    // GL 3.2 + GLSL 150
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+
    // only glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // 3.0+ only
#endif

    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
    glfwWindowHint(GLFW_TITLEBAR, GLFW_FALSE);
    //glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
}


int renderer_subsystem::init_glfw()
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    set_default_window_hints();

    return 0;
}

int renderer_subsystem::init_imgui()
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;    // Enable Multi-Viewport / Platform

    // Styling

    ImGuiStyle* style = &ImGui::GetStyle();

    static constexpr float padding             = 12.0f;
    static constexpr ImVec2 noPadding          = ImVec2(0, 0);
    static constexpr ImVec2 paddingVec         = ImVec2(padding, padding);
    static constexpr ImVec2 verticalPaddingVec = ImVec2(0, padding);

    static constexpr float rounding        = 6.0f;
    static constexpr float largerounding   = rounding * 2;
    static constexpr float largestRounding = rounding * 3;
    static constexpr float noRounding      = 2.0f;

    style->Alpha         = 1.0f;
    style->DisabledAlpha = 0.5f;

    style->WindowPadding = ImVec2(padding, padding / 2);
    style->FramePadding  = paddingVec;

    style->WindowRounding    = rounding;
    style->ChildRounding     = 0.0f;
    style->PopupRounding     = rounding;
    style->FrameRounding     = rounding;
    style->ScrollbarRounding = rounding;
    style->GrabRounding      = rounding;
    style->TabRounding       = rounding;

    style->SeparatorTextPadding;
    style->DisplayWindowPadding = paddingVec;
    style->DisplaySafeAreaPadding;
    style->CellPadding       = paddingVec;
    style->TouchExtraPadding = ImVec2(0, 0);

    style->WindowBorderSize         = 0.0f;
    style->WindowMinSize            = ImVec2(100, 100);
    style->WindowTitleAlign         = ImVec2(0.0f, 0.0f);
    style->WindowMenuButtonPosition = ImGuiDir_Right;
    style->ChildBorderSize          = 1.0f;
    style->PopupBorderSize          = 0.0f;
    style->FrameBorderSize          = 0.0f;
    style->ItemSpacing              = ImVec2(14, 8);
    style->ItemInnerSpacing         = ImVec2(0, 0);
    style->IndentSpacing            = 24.0f;
    style->ColumnsMinSpacing        = 10.0f;
    style->ScrollbarSize            = 18.0f;
    style->GrabMinSize              = 12.0f;
    style->LogSliderDeadzone;
    style->TabBorderSize             = 1.0f;
    style->TabMinWidthForCloseButton = 0.0f;
    style->TabBarBorderSize          = 1.0f;
    style->TableAngledHeadersAngle   = 45.0f;
    style->ColorButtonPosition;
    style->ButtonTextAlign         = ImVec2(0.0f, 0.5f);
    style->SelectableTextAlign     = ImVec2(0.5f, 0.5f);
    style->SeparatorTextBorderSize = 1.0f;
    style->SeparatorTextAlign      = ImVec2(0.5f, 0.5f);
    style->DockingSeparatorSize    = 4.0f;
    style->MouseCursorScale;
    style->AntiAliasedLines;
    style->AntiAliasedLinesUseTex;
    style->AntiAliasedFill;
    style->CurveTessellationTol;
    style->CircleTessellationMaxError;

    theme::carbon_g100::apply_theme();

    return 0;
}

int renderer_subsystem::init()
{
    if (init_glfw() == 0)
    {
        if (init_imgui() == 0)
        {
            // Create window with graphics context
            m_window = window_guard(1920, 1080, "Sound Bakery");

            return 0;
        }
    }
    return 1;
}

void renderer_subsystem::pre_tick(double deltaTime)
{
    if (glfwWindowShouldClose(m_window))
    {
        get_app()->request_exit();
    }
    else
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }
}

void renderer_subsystem::tick(double deltaTime) {}

void renderer_subsystem::tick_rendering(double deltaTime)
{
    static ImVec4 clear_color = ImVec4(255.0f, 100.0f, 180.0f, 1.00f);

    // Rendering
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(m_window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w,
                 clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
    glfwMakeContextCurrent(m_window);

    glfwSwapBuffers(m_window);
}

void renderer_subsystem::exit()
{
    ImGui::GetIO().Fonts->Clear();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
}

void renderer_subsystem::set_window_title(const std::string& title) { m_window.set_title(title); }

void gluten::renderer_subsystem::toggle_minimised() 
{
    if (is_minimised())
    {
        glfwRestoreWindow(m_window.m_window);
    }
    else
    {
        glfwIconifyWindow(m_window.m_window);
    }
}

void gluten::renderer_subsystem::toggle_maximised() 
{
    if (is_maximized())
    {
        glfwRestoreWindow(m_window.m_window);
    }
    else
    {
        glfwMaximizeWindow(m_window.m_window);
    }
}

bool gluten::renderer_subsystem::is_minimised() const
{
    return (bool)glfwGetWindowAttrib(m_window.m_window, GLFW_ICONIFIED);
}

bool gluten::renderer_subsystem::is_maximized() const
{
    return (bool)glfwGetWindowAttrib(m_window.m_window, GLFW_MAXIMIZED);
}