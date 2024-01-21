#include "RendererSubsystem.h"

#include "App/App.h"
#include "IconsFontAwesome6.h"
#include "IconsFontaudio.h"
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

RendererSubsystem::WindowGuard::WindowGuard(int width,
                                            int height,
                                            const std::string& windowName)
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

    ImGui_ImplGlfw_InitForOpenGL(m_window, true);

    // If ImGui backend is not created, make it
    if (ImGui::GetIO().BackendRendererUserData == nullptr)
    {
        ImGui_ImplOpenGL3_Init(glslVersion::glslVersion);
    }
}

RendererSubsystem::WindowGuard::WindowGuard(WindowGuard&& other) noexcept
{
    m_window       = other.m_window;
    other.m_window = nullptr;
}

RendererSubsystem::WindowGuard& RendererSubsystem::WindowGuard::operator=(
    WindowGuard&& other) noexcept
{
    if (this != &other)
    {
        m_window       = other.m_window;
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

int RendererSubsystem::PreInit(int ArgC, char* ArgV[]) { return 0; }

void RendererSubsystem::SetDefaultWindowHints()
{
    // Decide GL+GLSL versions
#if defined(__APPLE__)
    // GL 3.2 + GLSL 150
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);  // Required on Mac
#else
    // GL 3.0 + GLSL 130
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+
    // only glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // 3.0+ only
#endif
}

static ImVec4 hexToRGB(ImU32 hex) 
{
    float s = 1.0f / 255.0f;
    return ImVec4(((hex >> IM_COL32_R_SHIFT) & 0xFF) * s,
                  ((hex >> IM_COL32_G_SHIFT) & 0xFF) * s,
                  ((hex >> IM_COL32_B_SHIFT) & 0xFF) * s,
                  ((hex >> IM_COL32_A_SHIFT) & 0xFF) * s);
}

static ImVec4 hexStringToRGB(const std::string& string)
{
    ImVec4 color = hexToRGB(static_cast<ImU32>(std::stoul(string, nullptr, 16)));
    color.w = 1.0f;
    return color;
}

static ImVec4 adjustAlpha(const ImVec4& color, const float alpha)
{
    ImVec4 resultColor = color;
    resultColor.w      = alpha;
    return resultColor;
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
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |=       ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |=       ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
    io.ConfigFlags |=       ImGuiConfigFlags_DockingEnable;     // Enable Docking
    io.ConfigFlags |=       ImGuiConfigFlags_ViewportsEnable;   // Enable Multi-Viewport / Platform

    // Styling

    ImGuiStyle* style = &ImGui::GetStyle();

    static constexpr float padding = 12.0f;
    static constexpr ImVec2 noPadding  = ImVec2(0, 0);
    static constexpr ImVec2 paddingVec = ImVec2(padding, padding);
    static constexpr ImVec2 verticalPaddingVec = ImVec2(0, padding);

    static constexpr float rounding         = 6.0f;
    static constexpr float largerounding    = rounding * 2;
    static constexpr float largestRounding  = rounding * 3;
    static constexpr float noRounding       = 2.0f;

    style->Alpha                    = 1.0f;                   
    style->DisabledAlpha            = 0.5f;   

    style->WindowPadding            = noPadding;
    style->FramePadding             = paddingVec;

    style->WindowRounding           = rounding;
    style->ChildRounding            = rounding;          
    style->PopupRounding            = rounding;          
    style->FrameRounding            = rounding;
    style->ScrollbarRounding        = rounding;
    style->GrabRounding             = rounding;
    style->TabRounding              = largestRounding;

    style->SeparatorTextPadding;       
    style->DisplayWindowPadding     = paddingVec;       
    style->DisplaySafeAreaPadding;     
    style->CellPadding              = paddingVec;
    style->TouchExtraPadding        = ImVec2(0, 0);        

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
    style->TabBorderSize            = 1.0f;              
    style->TabMinWidthForCloseButton= 0.0f;
    style->TabBarBorderSize         = 1.0f;          
    style->TableAngledHeadersAngle  = 45.0f;    
    style->ColorButtonPosition;        
    style->ButtonTextAlign          = ImVec2(0.0f, 0.5f);           
    style->SelectableTextAlign      = ImVec2(0.5f, 0.5f);   
    style->SeparatorTextBorderSize = 1.0f;
    style->SeparatorTextAlign       = ImVec2(0.5f, 0.5f);            
    style->DockingSeparatorSize     = 4.0f;   
    style->MouseCursorScale;           
    style->AntiAliasedLines;           
    style->AntiAliasedLinesUseTex;     
    style->AntiAliasedFill;            
    style->CurveTessellationTol;       
    style->CircleTessellationMaxError; 

    #pragma region "Carbon"

    static ImVec4 black         = hexStringToRGB("000000");
    static ImVec4 black100      = black;
    static ImVec4 blackHover    = hexStringToRGB("212121");

    static ImVec4 white         = hexStringToRGB("ffffff");
    static ImVec4 white0        = white;
    static ImVec4 whiteHover    = hexStringToRGB("e8e8e8");

    static ImVec4 yellow10      = hexStringToRGB("fcf4d6");
    static ImVec4 yellow20      = hexStringToRGB("fddc69");
    static ImVec4 yellow30      = hexStringToRGB("f1c21b");
    static ImVec4 yellow40      = hexStringToRGB("d2a106");
    static ImVec4 yellow50      = hexStringToRGB("b28600");
    static ImVec4 yellow60      = hexStringToRGB("8e6a00");
    static ImVec4 yellow70      = hexStringToRGB("684e00");
    static ImVec4 yellow80      = hexStringToRGB("483700");
    static ImVec4 yellow90      = hexStringToRGB("302400");
    static ImVec4 yellow100     = hexStringToRGB("1c1500");

    static ImVec4 yellow10Hover  = hexStringToRGB("f8e6a0");
    static ImVec4 yellow20Hover  = hexStringToRGB("fccd27");
    static ImVec4 yellow30Hover  = hexStringToRGB("ddb00e");
    static ImVec4 yellow40Hover  = hexStringToRGB("bc9005");
    static ImVec4 yellow50Hover  = hexStringToRGB("9e7700");
    static ImVec4 yellow60Hover  = hexStringToRGB("755800");
    static ImVec4 yellow70Hover  = hexStringToRGB("806000");
    static ImVec4 yellow80Hover  = hexStringToRGB("5c4600");
    static ImVec4 yellow90Hover  = hexStringToRGB("3d2e00");
    static ImVec4 yellow100Hover = hexStringToRGB("332600");
   
    static ImVec4 orange10  = hexStringToRGB("fff2e8");
    static ImVec4 orange20  = hexStringToRGB("ffd9be");
    static ImVec4 orange30  = hexStringToRGB("ffb784");
    static ImVec4 orange40  = hexStringToRGB("ff832b");
    static ImVec4 orange50  = hexStringToRGB("eb6200");
    static ImVec4 orange60  = hexStringToRGB("ba4e00");
    static ImVec4 orange70  = hexStringToRGB("8a3800");
    static ImVec4 orange80  = hexStringToRGB("5e2900");
    static ImVec4 orange90  = hexStringToRGB("3e1a00");
    static ImVec4 orange100 = hexStringToRGB("231000");

    static ImVec4 orange10Hover  = hexStringToRGB("ffe2cc");
    static ImVec4 orange20Hover  = hexStringToRGB("ffc69e");
    static ImVec4 orange30Hover  = hexStringToRGB("ff9d57");
    static ImVec4 orange40Hover  = hexStringToRGB("fa6800");
    static ImVec4 orange50Hover  = hexStringToRGB("cc5500");
    static ImVec4 orange60Hover  = hexStringToRGB("9e4200");
    static ImVec4 orange70Hover  = hexStringToRGB("a84400");
    static ImVec4 orange80Hover  = hexStringToRGB("753300");
    static ImVec4 orange90Hover  = hexStringToRGB("522200");
    static ImVec4 orange100Hover = hexStringToRGB("421e00");

    static ImVec4 red10  = hexStringToRGB("fff1f1");
    static ImVec4 red20  = hexStringToRGB("ffd7d9");
    static ImVec4 red30  = hexStringToRGB("ffb3b8");
    static ImVec4 red40  = hexStringToRGB("ff8389");
    static ImVec4 red50  = hexStringToRGB("fa4d56");
    static ImVec4 red60  = hexStringToRGB("da1e28");
    static ImVec4 red70  = hexStringToRGB("a2191f");
    static ImVec4 red80  = hexStringToRGB("750e13");
    static ImVec4 red90  = hexStringToRGB("520408");
    static ImVec4 red100 = hexStringToRGB("2d0709");

    static ImVec4 red100Hover = hexStringToRGB("540d11");
    static ImVec4 red90Hover  = hexStringToRGB("66050a");
    static ImVec4 red80Hover  = hexStringToRGB("921118");
    static ImVec4 red70Hover  = hexStringToRGB("c21e25");
    static ImVec4 red60Hover  = hexStringToRGB("b81922");
    static ImVec4 red50Hover  = hexStringToRGB("ee0713");
    static ImVec4 red40Hover  = hexStringToRGB("ff6168");
    static ImVec4 red30Hover  = hexStringToRGB("ff99a0");
    static ImVec4 red20Hover  = hexStringToRGB("ffc2c5");
    static ImVec4 red10Hover  = hexStringToRGB("ffe0e0");

    static ImVec4 magenta10  = hexStringToRGB("fff0f7");
    static ImVec4 magenta20  = hexStringToRGB("ffd6e8");
    static ImVec4 magenta30  = hexStringToRGB("ffafd2");
    static ImVec4 magenta40  = hexStringToRGB("ff7eb6");
    static ImVec4 magenta50  = hexStringToRGB("ee5396");
    static ImVec4 magenta60  = hexStringToRGB("d02670");
    static ImVec4 magenta70  = hexStringToRGB("9f1853");
    static ImVec4 magenta80  = hexStringToRGB("740937");
    static ImVec4 magenta90  = hexStringToRGB("510224");
    static ImVec4 magenta100 = hexStringToRGB("2a0a18");

    static ImVec4 magenta100Hover = hexStringToRGB("53142f");
    static ImVec4 magenta90Hover  = hexStringToRGB("68032e");
    static ImVec4 magenta80Hover  = hexStringToRGB("8e0b43");
    static ImVec4 magenta70Hover  = hexStringToRGB("bf1d63");
    static ImVec4 magenta60Hover  = hexStringToRGB("b0215f");
    static ImVec4 magenta50Hover  = hexStringToRGB("e3176f");
    static ImVec4 magenta40Hover  = hexStringToRGB("ff57a0");
    static ImVec4 magenta30Hover  = hexStringToRGB("ff94c3");
    static ImVec4 magenta20Hover  = hexStringToRGB("ffbdda");
    static ImVec4 magenta10Hover  = hexStringToRGB("ffe0ef");

    static ImVec4 purple10  = hexStringToRGB("f6f2ff");
    static ImVec4 purple20  = hexStringToRGB("e8daff");
    static ImVec4 purple30  = hexStringToRGB("d4bbff");
    static ImVec4 purple40  = hexStringToRGB("be95ff");
    static ImVec4 purple50  = hexStringToRGB("a56eff");
    static ImVec4 purple60  = hexStringToRGB("8a3ffc");
    static ImVec4 purple70  = hexStringToRGB("6929c4");
    static ImVec4 purple80  = hexStringToRGB("491d8b");
    static ImVec4 purple90  = hexStringToRGB("31135e");
    static ImVec4 purple100 = hexStringToRGB("1c0f30");

    static ImVec4 purple100Hover = hexStringToRGB("341c59");
    static ImVec4 purple90Hover  = hexStringToRGB("40197b");
    static ImVec4 purple80Hover  = hexStringToRGB("5b24ad");
    static ImVec4 purple70Hover  = hexStringToRGB("7c3dd6");
    static ImVec4 purple60Hover  = hexStringToRGB("7822fb");
    static ImVec4 purple50Hover  = hexStringToRGB("9352ff");
    static ImVec4 purple40Hover  = hexStringToRGB("ae7aff");
    static ImVec4 purple30Hover  = hexStringToRGB("c5a3ff");
    static ImVec4 purple20Hover  = hexStringToRGB("dcc7ff");
    static ImVec4 purple10Hover  = hexStringToRGB("ede5ff");

    static ImVec4 blue10  = hexStringToRGB("edf5ff");
    static ImVec4 blue20  = hexStringToRGB("d0e2ff");
    static ImVec4 blue30  = hexStringToRGB("a6c8ff");
    static ImVec4 blue40  = hexStringToRGB("78a9ff");
    static ImVec4 blue50  = hexStringToRGB("4589ff");
    static ImVec4 blue60  = hexStringToRGB("0f62fe");
    static ImVec4 blue70  = hexStringToRGB("0043ce");
    static ImVec4 blue80  = hexStringToRGB("002d9c");
    static ImVec4 blue90  = hexStringToRGB("001d6c");
    static ImVec4 blue100 = hexStringToRGB("001141");

    static ImVec4 blue100Hover = hexStringToRGB("001f75");
    static ImVec4 blue90Hover  = hexStringToRGB("00258a");
    static ImVec4 blue80Hover  = hexStringToRGB("0039c7");
    static ImVec4 blue70Hover  = hexStringToRGB("0053ff");
    static ImVec4 blue60Hover  = hexStringToRGB("0050e6");
    static ImVec4 blue50Hover  = hexStringToRGB("1f70ff");
    static ImVec4 blue40Hover  = hexStringToRGB("5c97ff");
    static ImVec4 blue30Hover  = hexStringToRGB("8ab6ff");
    static ImVec4 blue20Hover  = hexStringToRGB("b8d3ff");
    static ImVec4 blue10Hover  = hexStringToRGB("dbebff");

    static ImVec4 cyan10  = hexStringToRGB("e5f6ff");
    static ImVec4 cyan20  = hexStringToRGB("bae6ff");
    static ImVec4 cyan30  = hexStringToRGB("82cfff");
    static ImVec4 cyan40  = hexStringToRGB("33b1ff");
    static ImVec4 cyan50  = hexStringToRGB("1192e8");
    static ImVec4 cyan60  = hexStringToRGB("0072c3");
    static ImVec4 cyan70  = hexStringToRGB("00539a");
    static ImVec4 cyan80  = hexStringToRGB("003a6d");
    static ImVec4 cyan90  = hexStringToRGB("012749");
    static ImVec4 cyan100 = hexStringToRGB("061727");

    static ImVec4 cyan10Hover  = hexStringToRGB("cceeff");
    static ImVec4 cyan20Hover  = hexStringToRGB("99daff");
    static ImVec4 cyan30Hover  = hexStringToRGB("57beff");
    static ImVec4 cyan40Hover  = hexStringToRGB("059fff");
    static ImVec4 cyan50Hover  = hexStringToRGB("0f7ec8");
    static ImVec4 cyan60Hover  = hexStringToRGB("005fa3");
    static ImVec4 cyan70Hover  = hexStringToRGB("0066bd");
    static ImVec4 cyan80Hover  = hexStringToRGB("00498a");
    static ImVec4 cyan90Hover  = hexStringToRGB("013360");
    static ImVec4 cyan100Hover = hexStringToRGB("0b2947");

    static ImVec4 teal10  = hexStringToRGB("d9fbfb");
    static ImVec4 teal20  = hexStringToRGB("9ef0f0");
    static ImVec4 teal30  = hexStringToRGB("3ddbd9");
    static ImVec4 teal40  = hexStringToRGB("08bdba");
    static ImVec4 teal50  = hexStringToRGB("009d9a");
    static ImVec4 teal60  = hexStringToRGB("007d79");
    static ImVec4 teal70  = hexStringToRGB("005d5d");
    static ImVec4 teal80  = hexStringToRGB("004144");
    static ImVec4 teal90  = hexStringToRGB("022b30");
    static ImVec4 teal100 = hexStringToRGB("081a1c");

    static ImVec4 teal10Hover  = hexStringToRGB("acf6f6");
    static ImVec4 teal20Hover  = hexStringToRGB("57e5e5");
    static ImVec4 teal30Hover  = hexStringToRGB("25cac8");
    static ImVec4 teal40Hover  = hexStringToRGB("07aba9");
    static ImVec4 teal50Hover  = hexStringToRGB("008a87");
    static ImVec4 teal60Hover  = hexStringToRGB("006b68");
    static ImVec4 teal70Hover  = hexStringToRGB("007070");
    static ImVec4 teal80Hover  = hexStringToRGB("005357");
    static ImVec4 teal90Hover  = hexStringToRGB("033940");
    static ImVec4 teal100Hover = hexStringToRGB("0f3034");

    static ImVec4 green10  = hexStringToRGB("defbe6");
    static ImVec4 green20  = hexStringToRGB("a7f0ba");
    static ImVec4 green30  = hexStringToRGB("6fdc8c");
    static ImVec4 green40  = hexStringToRGB("42be65");
    static ImVec4 green50  = hexStringToRGB("24a148");
    static ImVec4 green60  = hexStringToRGB("198038");
    static ImVec4 green70  = hexStringToRGB("0e6027");
    static ImVec4 green80  = hexStringToRGB("044317");
    static ImVec4 green90  = hexStringToRGB("022d0d");
    static ImVec4 green100 = hexStringToRGB("071908");

    static ImVec4 green10Hover  = hexStringToRGB("b6f6c8");
    static ImVec4 green20Hover  = hexStringToRGB("74e792");
    static ImVec4 green30Hover  = hexStringToRGB("36ce5e");
    static ImVec4 green40Hover  = hexStringToRGB("3bab5a");
    static ImVec4 green50Hover  = hexStringToRGB("208e3f");
    static ImVec4 green60Hover  = hexStringToRGB("166f31");
    static ImVec4 green70Hover  = hexStringToRGB("11742f");
    static ImVec4 green80Hover  = hexStringToRGB("05521c");
    static ImVec4 green90Hover  = hexStringToRGB("033b11");
    static ImVec4 green100Hover = hexStringToRGB("0d300f");

    static ImVec4 coolGray10  = hexStringToRGB("f2f4f8");
    static ImVec4 coolGray20  = hexStringToRGB("dde1e6");
    static ImVec4 coolGray30  = hexStringToRGB("c1c7cd");
    static ImVec4 coolGray40  = hexStringToRGB("a2a9b0");
    static ImVec4 coolGray50  = hexStringToRGB("878d96");
    static ImVec4 coolGray60  = hexStringToRGB("697077");
    static ImVec4 coolGray70  = hexStringToRGB("4d5358");
    static ImVec4 coolGray80  = hexStringToRGB("343a3f");
    static ImVec4 coolGray90  = hexStringToRGB("21272a");
    static ImVec4 coolGray100 = hexStringToRGB("121619");

    static ImVec4 coolGray10Hover  = hexStringToRGB("e4e9f1");
    static ImVec4 coolGray20Hover  = hexStringToRGB("cdd3da");
    static ImVec4 coolGray30Hover  = hexStringToRGB("adb5bd");
    static ImVec4 coolGray40Hover  = hexStringToRGB("9199a1");
    static ImVec4 coolGray50Hover  = hexStringToRGB("757b85");
    static ImVec4 coolGray60Hover  = hexStringToRGB("585e64");
    static ImVec4 coolGray70Hover  = hexStringToRGB("5d646a");
    static ImVec4 coolGray80Hover  = hexStringToRGB("434a51");
    static ImVec4 coolGray90Hover  = hexStringToRGB("2b3236");
    static ImVec4 coolGray100Hover = hexStringToRGB("222a2f");

    static ImVec4 gray10  = hexStringToRGB("f4f4f4");
    static ImVec4 gray20  = hexStringToRGB("e0e0e0");
    static ImVec4 gray30  = hexStringToRGB("c6c6c6");
    static ImVec4 gray40  = hexStringToRGB("a8a8a8");
    static ImVec4 gray50  = hexStringToRGB("8d8d8d");
    static ImVec4 gray60  = hexStringToRGB("6f6f6f");
    static ImVec4 gray70  = hexStringToRGB("525252");
    static ImVec4 gray80  = hexStringToRGB("393939");
    static ImVec4 gray90  = hexStringToRGB("262626");
    static ImVec4 gray100 = hexStringToRGB("161616");

    static ImVec4 gray10Hover  = hexStringToRGB("e8e8e8");
    static ImVec4 gray20Hover  = hexStringToRGB("d1d1d1");
    static ImVec4 gray30Hover  = hexStringToRGB("b5b5b5");
    static ImVec4 gray40Hover  = hexStringToRGB("999999");
    static ImVec4 gray50Hover  = hexStringToRGB("7a7a7a");
    static ImVec4 gray60Hover  = hexStringToRGB("5e5e5e");
    static ImVec4 gray70Hover  = hexStringToRGB("636363");
    static ImVec4 gray80Hover  = hexStringToRGB("474747");
    static ImVec4 gray90Hover  = hexStringToRGB("333333");
    static ImVec4 gray100Hover = hexStringToRGB("292929");

    static ImVec4 warmGray10  = hexStringToRGB("f7f3f2");
    static ImVec4 warmGray20  = hexStringToRGB("e5e0df");
    static ImVec4 warmGray30  = hexStringToRGB("cac5c4");
    static ImVec4 warmGray40  = hexStringToRGB("ada8a8");
    static ImVec4 warmGray50  = hexStringToRGB("8f8b8b");
    static ImVec4 warmGray60  = hexStringToRGB("726e6e");
    static ImVec4 warmGray70  = hexStringToRGB("565151");
    static ImVec4 warmGray80  = hexStringToRGB("3c3838");
    static ImVec4 warmGray90  = hexStringToRGB("272525");
    static ImVec4 warmGray100 = hexStringToRGB("171414");

    static ImVec4 warmGray10Hover  = hexStringToRGB("f0e8e6");
    static ImVec4 warmGray20Hover  = hexStringToRGB("d8d0cf");
    static ImVec4 warmGray30Hover  = hexStringToRGB("b9b3b1");
    static ImVec4 warmGray40Hover  = hexStringToRGB("9c9696");
    static ImVec4 warmGray50Hover  = hexStringToRGB("7f7b7b");
    static ImVec4 warmGray60Hover  = hexStringToRGB("605d5d");
    static ImVec4 warmGray70Hover  = hexStringToRGB("696363");
    static ImVec4 warmGray80Hover  = hexStringToRGB("4c4848");
    static ImVec4 warmGray90Hover  = hexStringToRGB("343232");
    static ImVec4 warmGray100Hover = hexStringToRGB("2c2626");

    // Background
    static ImVec4 background              = gray100;
    static ImVec4 backgroundInverse       = gray10;
    static ImVec4 backgroundBrand         = blue60;
    static ImVec4 backgroundActive        = adjustAlpha(gray50, 0.4);
    static ImVec4 backgroundHover         = adjustAlpha(gray50, 0.16);
    static ImVec4 backgroundInverseHover  = gray10Hover;
    static ImVec4 backgroundSelected      = adjustAlpha(gray50, 0.24);
    static ImVec4 backgroundSelectedHover = adjustAlpha(gray50, 0.32);

    // Layer
    // layer-01
    static ImVec4 layer01              = gray90;
    static ImVec4 layerActive01        = gray70;
    static ImVec4 layerHover01         = gray90Hover;
    static ImVec4 layerSelected01      = gray80;
    static ImVec4 layerSelectedHover01 = gray80Hover;

    // layer-02
    static ImVec4 layer02              = gray80;
    static ImVec4 layerActive02        = gray60;
    static ImVec4 layerHover02         = gray80Hover;
    static ImVec4 layerSelected02      = gray70;
    static ImVec4 layerSelectedHover02 = gray70Hover;

    // layer-03
    static ImVec4 layer03              = gray70;
    static ImVec4 layerActive03        = gray50;
    static ImVec4 layerHover03         = gray70Hover;
    static ImVec4 layerSelected03      = gray60;
    static ImVec4 layerSelectedHover03 = gray60Hover;

    // layer
    static ImVec4 layerSelectedInverse  = gray10;
    static ImVec4 layerSelectedDisabled = gray40;

    // layer-accent-01
    static ImVec4 layerAccent01       = gray80;
    static ImVec4 layerAccentActive01 = gray60;
    static ImVec4 layerAccentHover01  = gray80Hover;

    // layer-accent-02
    static ImVec4 layerAccent02       = gray70;
    static ImVec4 layerAccentActive02 = gray50;
    static ImVec4 layerAccentHover02  = gray70Hover;

    // layer-accent-03
    static ImVec4 layerAccent03       = gray60;
    static ImVec4 layerAccentActive03 = gray80;
    static ImVec4 layerAccentHover03  = gray60Hover;

    // Field
    // field-01
    static ImVec4 field01      = gray90;
    static ImVec4 fieldHover01 = gray90Hover;

    // field-02
    static ImVec4 field02      = gray80;
    static ImVec4 fieldHover02 = gray80Hover;

    // field-03
    static ImVec4 field03      = gray70;
    static ImVec4 fieldHover03 = gray70Hover;

    // Border
    // border-subtle-00
    static ImVec4 borderSubtle00 = gray80;

    // border-subtle-01
    static ImVec4 borderSubtle01         = gray70;
    static ImVec4 borderSubtleSelected01 = gray60;

    // border-subtle-02
    static ImVec4 borderSubtle02         = gray60;
    static ImVec4 borderSubtleSelected02 = gray50;

    // border-subtle-03
    static ImVec4 borderSubtle03         = gray60;
    static ImVec4 borderSubtleSelected03 = gray50;

    // border-strong
    static ImVec4 borderStrong01 = gray60;
    static ImVec4 borderStrong02 = gray50;
    static ImVec4 borderStrong03 = gray40;

    // border-tile
    static ImVec4 borderTile01 = gray70;
    static ImVec4 borderTile02 = gray60;
    static ImVec4 borderTile03 = gray50;

    // border-inverse
    static ImVec4 borderInverse = gray10;

    // border-interactive
    static ImVec4 borderInteractive = blue50;

    // border
    static ImVec4 borderDisabled = adjustAlpha(gray50, 0.5f);

    // Text
    static ImVec4 textPrimary         = gray10;
    static ImVec4 textSecondary       = gray30;
    static ImVec4 textPlaceholder     = adjustAlpha(textPrimary, 0.4f);
    static ImVec4 textHelper          = gray40;
    static ImVec4 textError           = red40;
    static ImVec4 textInverse         = gray100;
    static ImVec4 textOnColor         = white;
    static ImVec4 textOnColorDisabled = adjustAlpha(textOnColor, 0.25f);
    static ImVec4 textDisabled        = adjustAlpha(textPrimary, 0.25f);

    // Link
    static ImVec4 linkPrimary       = blue40;
    static ImVec4 linkPrimaryHover  = blue30;
    static ImVec4 linkSecondary     = blue30;
    static ImVec4 linkInverse       = blue60;
    static ImVec4 linkVisited       = purple40;
    static ImVec4 linkInverseActive = gray100;
    static ImVec4 linkInverseHover  = blue70;

    // Icon
    static ImVec4 iconPrimary         = gray10;
    static ImVec4 iconSecondary       = gray30;
    static ImVec4 iconInverse         = gray100;
    static ImVec4 iconOnColor         = white;
    static ImVec4 iconOnColorDisabled = adjustAlpha(iconOnColor, 0.25f);
    static ImVec4 iconDisabled        = adjustAlpha(iconPrimary, 0.25f);
    static ImVec4 iconInteractive     = white;

    // Support
    static ImVec4 supportError            = red50;
    static ImVec4 supportSuccess          = green40;
    static ImVec4 supportWarning          = yellow30;
    static ImVec4 supportInfo             = blue50;
    static ImVec4 supportErrorInverse     = red60;
    static ImVec4 supportSuccessInverse   = green50;
    static ImVec4 supportWarningInverse   = yellow30;
    static ImVec4 supportInfoInverse      = blue70;
    static ImVec4 supportCautionMinor     = yellow30;
    static ImVec4 supportCautionMajor     = orange40;
    static ImVec4 supportCautionUndefined = purple50;

    // Focus
    static ImVec4 focus        = white;
    static ImVec4 focusInset   = gray100;
    static ImVec4 focusInverse = blue60;

    // Misc
    static ImVec4 interactive = blue50;
    static ImVec4 highlight   = blue80;
    static ImVec4 overlay     = adjustAlpha(black, 0.65);
    static ImVec4 toggleOff   = gray60;
    static ImVec4 shadow      = adjustAlpha(black, 0.8);

    #pragma endregion

    static const ImVec4 missingColorColor = magenta50;

    style->Colors[ImGuiCol_Text]                    = textPrimary;
    style->Colors[ImGuiCol_TextDisabled]            = adjustAlpha(textPrimary, 0.6f);

    style->Colors[ImGuiCol_TitleBg]                 = background;
    style->Colors[ImGuiCol_MenuBarBg]               = background;  
    style->Colors[ImGuiCol_WindowBg]                = layer01;
    style->Colors[ImGuiCol_ChildBg]                 = layer02;
    style->Colors[ImGuiCol_FrameBg]                 = layer03;

    style->Colors[ImGuiCol_PopupBg]                 = layer03;
    style->Colors[ImGuiCol_ScrollbarBg]             = layer03;
    style->Colors[ImGuiCol_DockingEmptyBg]          = layer03;
    style->Colors[ImGuiCol_TableRowBg]              = layer03;
    style->Colors[ImGuiCol_TextSelectedBg]          = layer03;
    style->Colors[ImGuiCol_TableHeaderBg]           = layer03;
    style->Colors[ImGuiCol_TextSelectedBg]          = layer03;

    style->Colors[ImGuiCol_TitleBgActive]           = background;
    style->Colors[ImGuiCol_TitleBgCollapsed]        = background;
    style->Colors[ImGuiCol_FrameBgHovered]          = backgroundHover;
    style->Colors[ImGuiCol_FrameBgActive]           = backgroundActive;

    style->Colors[ImGuiCol_TableRowBgAlt]           = missingColorColor;
    style->Colors[ImGuiCol_NavWindowingDimBg]       = missingColorColor;
    style->Colors[ImGuiCol_ModalWindowDimBg]        = missingColorColor;

    style->Colors[ImGuiCol_Border]                  = background;
    style->Colors[ImGuiCol_BorderShadow]            = shadow;
    style->Colors[ImGuiCol_ScrollbarGrab]           = missingColorColor;
    style->Colors[ImGuiCol_ScrollbarGrabHovered]    = missingColorColor;
    style->Colors[ImGuiCol_ScrollbarGrabActive]     = missingColorColor;
    style->Colors[ImGuiCol_CheckMark]               = missingColorColor;
    style->Colors[ImGuiCol_SliderGrab]              = missingColorColor;
    style->Colors[ImGuiCol_SliderGrabActive]        = missingColorColor;
    style->Colors[ImGuiCol_Button]                  = missingColorColor;
    style->Colors[ImGuiCol_ButtonHovered]           = missingColorColor;
    style->Colors[ImGuiCol_ButtonActive]            = textPrimary;
    style->Colors[ImGuiCol_Header]                  = missingColorColor;
    style->Colors[ImGuiCol_HeaderHovered]           = missingColorColor;
    style->Colors[ImGuiCol_HeaderActive]            = missingColorColor;
    style->Colors[ImGuiCol_ResizeGrip]              = missingColorColor;
    style->Colors[ImGuiCol_ResizeGripHovered]       = missingColorColor;
    style->Colors[ImGuiCol_ResizeGripActive]        = missingColorColor;
    style->Colors[ImGuiCol_PlotLines]               = missingColorColor;
    style->Colors[ImGuiCol_PlotLinesHovered]        = missingColorColor;
    style->Colors[ImGuiCol_PlotHistogram]           = missingColorColor;
    style->Colors[ImGuiCol_PlotHistogramHovered]    = missingColorColor;
    style->Colors[ImGuiCol_Separator]               = missingColorColor;
    style->Colors[ImGuiCol_SeparatorHovered]        = missingColorColor;
    style->Colors[ImGuiCol_SeparatorActive]         = missingColorColor;

    style->Colors[ImGuiCol_Tab]                     = layer01;
    style->Colors[ImGuiCol_TabHovered]              = layerHover01;

    style->Colors[ImGuiCol_TabActive]               = layer02;

    style->Colors[ImGuiCol_TabUnfocused]            = style->Colors[ImGuiCol_Tab];
    style->Colors[ImGuiCol_TabUnfocusedActive]      = style->Colors[ImGuiCol_TabActive];

    style->Colors[ImGuiCol_TableBorderStrong]       = layerAccent02;
    style->Colors[ImGuiCol_TableBorderLight]        = layerAccent01;

    style->Colors[ImGuiCol_DockingPreview]          = missingColorColor;
    style->Colors[ImGuiCol_DragDropTarget]          = missingColorColor;
    style->Colors[ImGuiCol_NavHighlight]            = missingColorColor;
    style->Colors[ImGuiCol_NavWindowingHighlight]   = missingColorColor;

    // Load Fonts
    const float baseFontSize = 18.0f;
    const float iconFontSize =
        baseFontSize * 2.0f /
        3.0f;  // FontAwesome fonts need to have their sizes reduced
               // by 2.0f/3.0f in order to align correctly

    static const ImWchar fontAwesomeIconRanges[] = {ICON_MIN_FA, ICON_MAX_16_FA,
                                                    0};
    static const ImWchar fontAudioIconRanges[] = {ICON_MIN_FAD, ICON_MAX_16_FAD,
                                                  0};

    ImFontConfig icons_config;
    icons_config.MergeMode        = true;
    icons_config.PixelSnapH       = true;
    icons_config.GlyphMinAdvanceX = iconFontSize;

    ImFont* mainFont = io.Fonts->AddFontFromFileTTF(
        m_app->GetResourceFilePath("fonts/Montserrat-Light.ttf").c_str(),
        baseFontSize);
    ImFont* fontAudio = io.Fonts->AddFontFromFileTTF(
        m_app->GetResourceFilePath("fonts/" FONT_ICON_FILE_NAME_FAD).c_str(),
        iconFontSize * 1.3f, &icons_config, fontAudioIconRanges);

    io.Fonts->AddFontFromFileTTF(
        m_app->GetResourceFilePath("fonts/Montserrat-Light.ttf").c_str(),
        baseFontSize);
    ImFont* fontAwesome = io.Fonts->AddFontFromFileTTF(
        m_app->GetResourceFilePath("fonts/" FONT_ICON_FILE_NAME_FAR).c_str(),
        iconFontSize, &icons_config, fontAwesomeIconRanges);

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

void RendererSubsystem::Tick(double deltaTime) {}

void RendererSubsystem::TickRendering(double deltaTime)
{
    static ImVec4 clear_color = ImVec4(255.0f, 100.0f, 180.0f, 1.00f);

    // Rendering
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(m_window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w,
                 clear_color.z * clear_color.w, clear_color.w);
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
