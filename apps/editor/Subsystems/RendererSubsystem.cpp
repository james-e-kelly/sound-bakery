#include "RendererSubsystem.h"

#include "App/App.h"

#include <stdio.h>

#include "IconsFontAwesome6.h"
#include "IconsFontaudio.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

namespace glslVersion
{
#if defined(__APPLE__)
    static const char* glslVersion = "#version 150";
#else
    static const char* glslVersion = "#version 130";
#endif
}

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

RendererSubsystem::WindowGuard::WindowGuard(int width, int height, const std::string& windowName)
{
    m_window = glfwCreateWindow(width, height, windowName.c_str(), NULL, NULL);
    assert(m_window);

    GLFWwindow* currentContext = glfwGetCurrentContext();

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1); // Enable vsync

    // Creating a window shouldn't auto steal the context
    if (currentContext != NULL)
    {
        glfwMakeContextCurrent(currentContext);
    }
    
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);

    // If ImGui backend is not created, make it
    if (ImGui::GetIO().BackendRendererUserData == nullptr)
    {
        ImGui_ImplOpenGL3_Init(glslVersion::glslVersion);
    }
}

RendererSubsystem::WindowGuard::WindowGuard(WindowGuard&& other) noexcept
{
    m_window = other.m_window;
    other.m_window = nullptr;
}

RendererSubsystem::WindowGuard& RendererSubsystem::WindowGuard::operator=(WindowGuard&& other) noexcept
{
    if (this != &other)
    {
        m_window = other.m_window;
        other.m_window = nullptr;
    }
    
    return *this;
}

RendererSubsystem::WindowGuard::~WindowGuard()
{
    if (m_window)
    {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
}

int RendererSubsystem::PreInit(int ArgC, char* ArgV[])
{
    return 0;
}

void RendererSubsystem::SetDefaultWindowHints()
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
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif
}

int RendererSubsystem::InitGLFW()
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    SetDefaultWindowHints();
    
    return 0;
}

int RendererSubsystem::InitImGui()
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
    
    // Load Fonts
    const float baseFontSize = 18.0f;
    const float iconFontSize = baseFontSize * 2.0f / 3.0f; // FontAwesome fonts need to have their sizes reduced by 2.0f/3.0f in order to align correctly
    
    static const ImWchar fontAwesomeIconRanges[] = { ICON_MIN_FA, ICON_MAX_16_FA, 0 };
    static const ImWchar fontAudioIconRanges[] = { ICON_MIN_FAD, ICON_MAX_16_FAD, 0 };
    
    ImFontConfig icons_config;
    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true;
    icons_config.GlyphMinAdvanceX = iconFontSize;
    
    ImFont* mainFont = io.Fonts->AddFontFromFileTTF(m_app->GetResourceFilePath("fonts/Montserrat-Light.ttf").c_str(), baseFontSize);
    ImFont* fontAudio = io.Fonts->AddFontFromFileTTF(m_app->GetResourceFilePath("fonts/" FONT_ICON_FILE_NAME_FAD).c_str(), iconFontSize * 1.3f, &icons_config, fontAudioIconRanges);
    
    io.Fonts->AddFontFromFileTTF(m_app->GetResourceFilePath("fonts/Montserrat-Light.ttf").c_str(), baseFontSize);
    ImFont* fontAwesome = io.Fonts->AddFontFromFileTTF(m_app->GetResourceFilePath( "fonts/" FONT_ICON_FILE_NAME_FAR).c_str(), iconFontSize, &icons_config, fontAwesomeIconRanges);
    
    return 0;
}

int RendererSubsystem::Init()
{
    if (InitGLFW() == 0)
    {
        if (InitImGui() == 0)
        {
            // Create window with graphics context
            m_window = WindowGuard(1920, 1080, "Sound Bakery");
            
            return 0;
        }
    }
    return 1;
}

void RendererSubsystem::PreTick(double deltaTime)
{
    if (glfwWindowShouldClose(m_window))
    {
        GetApp()->RequestExit();
    }
    else
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }
}

void RendererSubsystem::Tick(double deltaTime)
{     

}

void RendererSubsystem::TickRendering(double deltaTime)
{
    static ImVec4 clear_color = ImVec4(255.0f, 100.0f, 180.0f, 1.00f);

    // Rendering
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(m_window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
    glfwMakeContextCurrent(m_window);

    glfwSwapBuffers(m_window);
}

void RendererSubsystem::Exit()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
}
