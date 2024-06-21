#include "renderer_subsystem.h"

#include "IconsFontAwesome6.h"
#include "IconsFontaudio.h"
#include "app/app.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <cmrc/cmrc.hpp>

CMRC_DECLARE(sbk::Fonts);

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
}

static ImVec4 hex_to_rgb(ImU32 hex)
{
    float s = 1.0f / 255.0f;

    // The RGB order is backwards here -> BGR
    // It was noted the "correct" order made blues look orange
    // Swapping created the correct colours
    return ImVec4(((hex >> IM_COL32_B_SHIFT) & 0xFF) * s, ((hex >> IM_COL32_G_SHIFT) & 0xFF) * s,
                  ((hex >> IM_COL32_R_SHIFT) & 0xFF) * s, ((hex >> IM_COL32_A_SHIFT) & 0xFF) * s);
}

static ImVec4 hex_string_to_rgb(const std::string& string)
{
    ImVec4 color = hex_to_rgb(static_cast<ImU32>(std::stoul(string, nullptr, 16)));
    color.w      = 1.0f;
    return color;
}

static ImVec4 adjust_alpha(const ImVec4& color, const float alpha)
{
    ImVec4 resultColor = color;
    resultColor.w      = alpha;
    return resultColor;
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
    (void)io;
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
    style->ChildRounding     = rounding;
    style->PopupRounding     = rounding;
    style->FrameRounding     = rounding;
    style->ScrollbarRounding = rounding;
    style->GrabRounding      = rounding;
    style->TabRounding       = largestRounding;

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

#pragma region "Carbon"

    static ImVec4 black      = hex_string_to_rgb("000000");
    static ImVec4 black100   = black;
    static ImVec4 blackHover = hex_string_to_rgb("212121");

    static ImVec4 white      = hex_string_to_rgb("ffffff");
    static ImVec4 white0     = white;
    static ImVec4 whiteHover = hex_string_to_rgb("e8e8e8");

    static ImVec4 yellow10  = hex_string_to_rgb("fcf4d6");
    static ImVec4 yellow20  = hex_string_to_rgb("fddc69");
    static ImVec4 yellow30  = hex_string_to_rgb("f1c21b");
    static ImVec4 yellow40  = hex_string_to_rgb("d2a106");
    static ImVec4 yellow50  = hex_string_to_rgb("b28600");
    static ImVec4 yellow60  = hex_string_to_rgb("8e6a00");
    static ImVec4 yellow70  = hex_string_to_rgb("684e00");
    static ImVec4 yellow80  = hex_string_to_rgb("483700");
    static ImVec4 yellow90  = hex_string_to_rgb("302400");
    static ImVec4 yellow100 = hex_string_to_rgb("1c1500");

    static ImVec4 yellow10Hover  = hex_string_to_rgb("f8e6a0");
    static ImVec4 yellow20Hover  = hex_string_to_rgb("fccd27");
    static ImVec4 yellow30Hover  = hex_string_to_rgb("ddb00e");
    static ImVec4 yellow40Hover  = hex_string_to_rgb("bc9005");
    static ImVec4 yellow50Hover  = hex_string_to_rgb("9e7700");
    static ImVec4 yellow60Hover  = hex_string_to_rgb("755800");
    static ImVec4 yellow70Hover  = hex_string_to_rgb("806000");
    static ImVec4 yellow80Hover  = hex_string_to_rgb("5c4600");
    static ImVec4 yellow90Hover  = hex_string_to_rgb("3d2e00");
    static ImVec4 yellow100Hover = hex_string_to_rgb("332600");

    static ImVec4 orange10  = hex_string_to_rgb("fff2e8");
    static ImVec4 orange20  = hex_string_to_rgb("ffd9be");
    static ImVec4 orange30  = hex_string_to_rgb("ffb784");
    static ImVec4 orange40  = hex_string_to_rgb("ff832b");
    static ImVec4 orange50  = hex_string_to_rgb("eb6200");
    static ImVec4 orange60  = hex_string_to_rgb("ba4e00");
    static ImVec4 orange70  = hex_string_to_rgb("8a3800");
    static ImVec4 orange80  = hex_string_to_rgb("5e2900");
    static ImVec4 orange90  = hex_string_to_rgb("3e1a00");
    static ImVec4 orange100 = hex_string_to_rgb("231000");

    static ImVec4 orange10Hover  = hex_string_to_rgb("ffe2cc");
    static ImVec4 orange20Hover  = hex_string_to_rgb("ffc69e");
    static ImVec4 orange30Hover  = hex_string_to_rgb("ff9d57");
    static ImVec4 orange40Hover  = hex_string_to_rgb("fa6800");
    static ImVec4 orange50Hover  = hex_string_to_rgb("cc5500");
    static ImVec4 orange60Hover  = hex_string_to_rgb("9e4200");
    static ImVec4 orange70Hover  = hex_string_to_rgb("a84400");
    static ImVec4 orange80Hover  = hex_string_to_rgb("753300");
    static ImVec4 orange90Hover  = hex_string_to_rgb("522200");
    static ImVec4 orange100Hover = hex_string_to_rgb("421e00");

    static ImVec4 red10  = hex_string_to_rgb("fff1f1");
    static ImVec4 red20  = hex_string_to_rgb("ffd7d9");
    static ImVec4 red30  = hex_string_to_rgb("ffb3b8");
    static ImVec4 red40  = hex_string_to_rgb("ff8389");
    static ImVec4 red50  = hex_string_to_rgb("fa4d56");
    static ImVec4 red60  = hex_string_to_rgb("da1e28");
    static ImVec4 red70  = hex_string_to_rgb("a2191f");
    static ImVec4 red80  = hex_string_to_rgb("750e13");
    static ImVec4 red90  = hex_string_to_rgb("520408");
    static ImVec4 red100 = hex_string_to_rgb("2d0709");

    static ImVec4 red100Hover = hex_string_to_rgb("540d11");
    static ImVec4 red90Hover  = hex_string_to_rgb("66050a");
    static ImVec4 red80Hover  = hex_string_to_rgb("921118");
    static ImVec4 red70Hover  = hex_string_to_rgb("c21e25");
    static ImVec4 red60Hover  = hex_string_to_rgb("b81922");
    static ImVec4 red50Hover  = hex_string_to_rgb("ee0713");
    static ImVec4 red40Hover  = hex_string_to_rgb("ff6168");
    static ImVec4 red30Hover  = hex_string_to_rgb("ff99a0");
    static ImVec4 red20Hover  = hex_string_to_rgb("ffc2c5");
    static ImVec4 red10Hover  = hex_string_to_rgb("ffe0e0");

    static ImVec4 magenta10  = hex_string_to_rgb("fff0f7");
    static ImVec4 magenta20  = hex_string_to_rgb("ffd6e8");
    static ImVec4 magenta30  = hex_string_to_rgb("ffafd2");
    static ImVec4 magenta40  = hex_string_to_rgb("ff7eb6");
    static ImVec4 magenta50  = hex_string_to_rgb("ee5396");
    static ImVec4 magenta60  = hex_string_to_rgb("d02670");
    static ImVec4 magenta70  = hex_string_to_rgb("9f1853");
    static ImVec4 magenta80  = hex_string_to_rgb("740937");
    static ImVec4 magenta90  = hex_string_to_rgb("510224");
    static ImVec4 magenta100 = hex_string_to_rgb("2a0a18");

    static ImVec4 magenta100Hover = hex_string_to_rgb("53142f");
    static ImVec4 magenta90Hover  = hex_string_to_rgb("68032e");
    static ImVec4 magenta80Hover  = hex_string_to_rgb("8e0b43");
    static ImVec4 magenta70Hover  = hex_string_to_rgb("bf1d63");
    static ImVec4 magenta60Hover  = hex_string_to_rgb("b0215f");
    static ImVec4 magenta50Hover  = hex_string_to_rgb("e3176f");
    static ImVec4 magenta40Hover  = hex_string_to_rgb("ff57a0");
    static ImVec4 magenta30Hover  = hex_string_to_rgb("ff94c3");
    static ImVec4 magenta20Hover  = hex_string_to_rgb("ffbdda");
    static ImVec4 magenta10Hover  = hex_string_to_rgb("ffe0ef");

    static ImVec4 purple10  = hex_string_to_rgb("f6f2ff");
    static ImVec4 purple20  = hex_string_to_rgb("e8daff");
    static ImVec4 purple30  = hex_string_to_rgb("d4bbff");
    static ImVec4 purple40  = hex_string_to_rgb("be95ff");
    static ImVec4 purple50  = hex_string_to_rgb("a56eff");
    static ImVec4 purple60  = hex_string_to_rgb("8a3ffc");
    static ImVec4 purple70  = hex_string_to_rgb("6929c4");
    static ImVec4 purple80  = hex_string_to_rgb("491d8b");
    static ImVec4 purple90  = hex_string_to_rgb("31135e");
    static ImVec4 purple100 = hex_string_to_rgb("1c0f30");

    static ImVec4 purple100Hover = hex_string_to_rgb("341c59");
    static ImVec4 purple90Hover  = hex_string_to_rgb("40197b");
    static ImVec4 purple80Hover  = hex_string_to_rgb("5b24ad");
    static ImVec4 purple70Hover  = hex_string_to_rgb("7c3dd6");
    static ImVec4 purple60Hover  = hex_string_to_rgb("7822fb");
    static ImVec4 purple50Hover  = hex_string_to_rgb("9352ff");
    static ImVec4 purple40Hover  = hex_string_to_rgb("ae7aff");
    static ImVec4 purple30Hover  = hex_string_to_rgb("c5a3ff");
    static ImVec4 purple20Hover  = hex_string_to_rgb("dcc7ff");
    static ImVec4 purple10Hover  = hex_string_to_rgb("ede5ff");

    static ImVec4 blue10  = hex_string_to_rgb("edf5ff");
    static ImVec4 blue20  = hex_string_to_rgb("d0e2ff");
    static ImVec4 blue30  = hex_string_to_rgb("a6c8ff");
    static ImVec4 blue40  = hex_string_to_rgb("78a9ff");
    static ImVec4 blue50  = hex_string_to_rgb("4589ff");
    static ImVec4 blue60  = hex_string_to_rgb("0f62fe");
    static ImVec4 blue70  = hex_string_to_rgb("0043ce");
    static ImVec4 blue80  = hex_string_to_rgb("002d9c");
    static ImVec4 blue90  = hex_string_to_rgb("001d6c");
    static ImVec4 blue100 = hex_string_to_rgb("001141");

    static ImVec4 blue100Hover = hex_string_to_rgb("001f75");
    static ImVec4 blue90Hover  = hex_string_to_rgb("00258a");
    static ImVec4 blue80Hover  = hex_string_to_rgb("0039c7");
    static ImVec4 blue70Hover  = hex_string_to_rgb("0053ff");
    static ImVec4 blue60Hover  = hex_string_to_rgb("0050e6");
    static ImVec4 blue50Hover  = hex_string_to_rgb("1f70ff");
    static ImVec4 blue40Hover  = hex_string_to_rgb("5c97ff");
    static ImVec4 blue30Hover  = hex_string_to_rgb("8ab6ff");
    static ImVec4 blue20Hover  = hex_string_to_rgb("b8d3ff");
    static ImVec4 blue10Hover  = hex_string_to_rgb("dbebff");

    static ImVec4 cyan10  = hex_string_to_rgb("e5f6ff");
    static ImVec4 cyan20  = hex_string_to_rgb("bae6ff");
    static ImVec4 cyan30  = hex_string_to_rgb("82cfff");
    static ImVec4 cyan40  = hex_string_to_rgb("33b1ff");
    static ImVec4 cyan50  = hex_string_to_rgb("1192e8");
    static ImVec4 cyan60  = hex_string_to_rgb("0072c3");
    static ImVec4 cyan70  = hex_string_to_rgb("00539a");
    static ImVec4 cyan80  = hex_string_to_rgb("003a6d");
    static ImVec4 cyan90  = hex_string_to_rgb("012749");
    static ImVec4 cyan100 = hex_string_to_rgb("061727");

    static ImVec4 cyan10Hover  = hex_string_to_rgb("cceeff");
    static ImVec4 cyan20Hover  = hex_string_to_rgb("99daff");
    static ImVec4 cyan30Hover  = hex_string_to_rgb("57beff");
    static ImVec4 cyan40Hover  = hex_string_to_rgb("059fff");
    static ImVec4 cyan50Hover  = hex_string_to_rgb("0f7ec8");
    static ImVec4 cyan60Hover  = hex_string_to_rgb("005fa3");
    static ImVec4 cyan70Hover  = hex_string_to_rgb("0066bd");
    static ImVec4 cyan80Hover  = hex_string_to_rgb("00498a");
    static ImVec4 cyan90Hover  = hex_string_to_rgb("013360");
    static ImVec4 cyan100Hover = hex_string_to_rgb("0b2947");

    static ImVec4 teal10  = hex_string_to_rgb("d9fbfb");
    static ImVec4 teal20  = hex_string_to_rgb("9ef0f0");
    static ImVec4 teal30  = hex_string_to_rgb("3ddbd9");
    static ImVec4 teal40  = hex_string_to_rgb("08bdba");
    static ImVec4 teal50  = hex_string_to_rgb("009d9a");
    static ImVec4 teal60  = hex_string_to_rgb("007d79");
    static ImVec4 teal70  = hex_string_to_rgb("005d5d");
    static ImVec4 teal80  = hex_string_to_rgb("004144");
    static ImVec4 teal90  = hex_string_to_rgb("022b30");
    static ImVec4 teal100 = hex_string_to_rgb("081a1c");

    static ImVec4 teal10Hover  = hex_string_to_rgb("acf6f6");
    static ImVec4 teal20Hover  = hex_string_to_rgb("57e5e5");
    static ImVec4 teal30Hover  = hex_string_to_rgb("25cac8");
    static ImVec4 teal40Hover  = hex_string_to_rgb("07aba9");
    static ImVec4 teal50Hover  = hex_string_to_rgb("008a87");
    static ImVec4 teal60Hover  = hex_string_to_rgb("006b68");
    static ImVec4 teal70Hover  = hex_string_to_rgb("007070");
    static ImVec4 teal80Hover  = hex_string_to_rgb("005357");
    static ImVec4 teal90Hover  = hex_string_to_rgb("033940");
    static ImVec4 teal100Hover = hex_string_to_rgb("0f3034");

    static ImVec4 green10  = hex_string_to_rgb("defbe6");
    static ImVec4 green20  = hex_string_to_rgb("a7f0ba");
    static ImVec4 green30  = hex_string_to_rgb("6fdc8c");
    static ImVec4 green40  = hex_string_to_rgb("42be65");
    static ImVec4 green50  = hex_string_to_rgb("24a148");
    static ImVec4 green60  = hex_string_to_rgb("198038");
    static ImVec4 green70  = hex_string_to_rgb("0e6027");
    static ImVec4 green80  = hex_string_to_rgb("044317");
    static ImVec4 green90  = hex_string_to_rgb("022d0d");
    static ImVec4 green100 = hex_string_to_rgb("071908");

    static ImVec4 green10Hover  = hex_string_to_rgb("b6f6c8");
    static ImVec4 green20Hover  = hex_string_to_rgb("74e792");
    static ImVec4 green30Hover  = hex_string_to_rgb("36ce5e");
    static ImVec4 green40Hover  = hex_string_to_rgb("3bab5a");
    static ImVec4 green50Hover  = hex_string_to_rgb("208e3f");
    static ImVec4 green60Hover  = hex_string_to_rgb("166f31");
    static ImVec4 green70Hover  = hex_string_to_rgb("11742f");
    static ImVec4 green80Hover  = hex_string_to_rgb("05521c");
    static ImVec4 green90Hover  = hex_string_to_rgb("033b11");
    static ImVec4 green100Hover = hex_string_to_rgb("0d300f");

    static ImVec4 coolGray10  = hex_string_to_rgb("f2f4f8");
    static ImVec4 coolGray20  = hex_string_to_rgb("dde1e6");
    static ImVec4 coolGray30  = hex_string_to_rgb("c1c7cd");
    static ImVec4 coolGray40  = hex_string_to_rgb("a2a9b0");
    static ImVec4 coolGray50  = hex_string_to_rgb("878d96");
    static ImVec4 coolGray60  = hex_string_to_rgb("697077");
    static ImVec4 coolGray70  = hex_string_to_rgb("4d5358");
    static ImVec4 coolGray80  = hex_string_to_rgb("343a3f");
    static ImVec4 coolGray90  = hex_string_to_rgb("21272a");
    static ImVec4 coolGray100 = hex_string_to_rgb("121619");

    static ImVec4 coolGray10Hover  = hex_string_to_rgb("e4e9f1");
    static ImVec4 coolGray20Hover  = hex_string_to_rgb("cdd3da");
    static ImVec4 coolGray30Hover  = hex_string_to_rgb("adb5bd");
    static ImVec4 coolGray40Hover  = hex_string_to_rgb("9199a1");
    static ImVec4 coolGray50Hover  = hex_string_to_rgb("757b85");
    static ImVec4 coolGray60Hover  = hex_string_to_rgb("585e64");
    static ImVec4 coolGray70Hover  = hex_string_to_rgb("5d646a");
    static ImVec4 coolGray80Hover  = hex_string_to_rgb("434a51");
    static ImVec4 coolGray90Hover  = hex_string_to_rgb("2b3236");
    static ImVec4 coolGray100Hover = hex_string_to_rgb("222a2f");

    static ImVec4 gray10  = hex_string_to_rgb("f4f4f4");
    static ImVec4 gray20  = hex_string_to_rgb("e0e0e0");
    static ImVec4 gray30  = hex_string_to_rgb("c6c6c6");
    static ImVec4 gray40  = hex_string_to_rgb("a8a8a8");
    static ImVec4 gray50  = hex_string_to_rgb("8d8d8d");
    static ImVec4 gray60  = hex_string_to_rgb("6f6f6f");
    static ImVec4 gray70  = hex_string_to_rgb("525252");
    static ImVec4 gray80  = hex_string_to_rgb("393939");
    static ImVec4 gray90  = hex_string_to_rgb("262626");
    static ImVec4 gray100 = hex_string_to_rgb("161616");

    static ImVec4 gray10Hover  = hex_string_to_rgb("e8e8e8");
    static ImVec4 gray20Hover  = hex_string_to_rgb("d1d1d1");
    static ImVec4 gray30Hover  = hex_string_to_rgb("b5b5b5");
    static ImVec4 gray40Hover  = hex_string_to_rgb("999999");
    static ImVec4 gray50Hover  = hex_string_to_rgb("7a7a7a");
    static ImVec4 gray60Hover  = hex_string_to_rgb("5e5e5e");
    static ImVec4 gray70Hover  = hex_string_to_rgb("636363");
    static ImVec4 gray80Hover  = hex_string_to_rgb("474747");
    static ImVec4 gray90Hover  = hex_string_to_rgb("333333");
    static ImVec4 gray100Hover = hex_string_to_rgb("292929");

    static ImVec4 warmGray10  = hex_string_to_rgb("f7f3f2");
    static ImVec4 warmGray20  = hex_string_to_rgb("e5e0df");
    static ImVec4 warmGray30  = hex_string_to_rgb("cac5c4");
    static ImVec4 warmGray40  = hex_string_to_rgb("ada8a8");
    static ImVec4 warmGray50  = hex_string_to_rgb("8f8b8b");
    static ImVec4 warmGray60  = hex_string_to_rgb("726e6e");
    static ImVec4 warmGray70  = hex_string_to_rgb("565151");
    static ImVec4 warmGray80  = hex_string_to_rgb("3c3838");
    static ImVec4 warmGray90  = hex_string_to_rgb("272525");
    static ImVec4 warmGray100 = hex_string_to_rgb("171414");

    static ImVec4 warmGray10Hover  = hex_string_to_rgb("f0e8e6");
    static ImVec4 warmGray20Hover  = hex_string_to_rgb("d8d0cf");
    static ImVec4 warmGray30Hover  = hex_string_to_rgb("b9b3b1");
    static ImVec4 warmGray40Hover  = hex_string_to_rgb("9c9696");
    static ImVec4 warmGray50Hover  = hex_string_to_rgb("7f7b7b");
    static ImVec4 warmGray60Hover  = hex_string_to_rgb("605d5d");
    static ImVec4 warmGray70Hover  = hex_string_to_rgb("696363");
    static ImVec4 warmGray80Hover  = hex_string_to_rgb("4c4848");
    static ImVec4 warmGray90Hover  = hex_string_to_rgb("343232");
    static ImVec4 warmGray100Hover = hex_string_to_rgb("2c2626");

    // Background
    static ImVec4 background              = gray100;
    static ImVec4 backgroundInverse       = gray10;
    static ImVec4 backgroundBrand         = blue60;
    static ImVec4 backgroundActive        = adjust_alpha(gray50, 0.4);
    static ImVec4 backgroundHover         = adjust_alpha(gray50, 0.16);
    static ImVec4 backgroundInverseHover  = gray10Hover;
    static ImVec4 backgroundSelected      = adjust_alpha(gray50, 0.24);
    static ImVec4 backgroundSelectedHover = adjust_alpha(gray50, 0.32);

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
    static ImVec4 borderDisabled = adjust_alpha(gray50, 0.5f);

    // Text
    static ImVec4 textPrimary         = gray10;
    static ImVec4 textSecondary       = gray30;
    static ImVec4 textPlaceholder     = adjust_alpha(textPrimary, 0.4f);
    static ImVec4 textHelper          = gray40;
    static ImVec4 textError           = red40;
    static ImVec4 textInverse         = gray100;
    static ImVec4 textOnColor         = white;
    static ImVec4 textOnColorDisabled = adjust_alpha(textOnColor, 0.25f);
    static ImVec4 textDisabled        = adjust_alpha(textPrimary, 0.25f);

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
    static ImVec4 iconOnColorDisabled = adjust_alpha(iconOnColor, 0.25f);
    static ImVec4 iconDisabled        = adjust_alpha(iconPrimary, 0.25f);
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
    static ImVec4 overlay     = adjust_alpha(black, 0.65);
    static ImVec4 toggleOff   = gray60;
    static ImVec4 shadow      = adjust_alpha(black, 0.8);

#pragma endregion

    static const ImVec4 missingColorColor = magenta50;

    style->Colors[ImGuiCol_Text]         = textPrimary;
    style->Colors[ImGuiCol_TextDisabled] = adjust_alpha(textPrimary, 0.6f);

    style->Colors[ImGuiCol_TitleBg]   = background;
    style->Colors[ImGuiCol_MenuBarBg] = background;
    style->Colors[ImGuiCol_WindowBg]  = layer01;
    style->Colors[ImGuiCol_ChildBg]   = layer02;
    style->Colors[ImGuiCol_FrameBg]   = layer03;

    style->Colors[ImGuiCol_PopupBg]        = layer03;
    style->Colors[ImGuiCol_ScrollbarBg]    = layer03;
    style->Colors[ImGuiCol_DockingEmptyBg] = layer03;
    style->Colors[ImGuiCol_TableRowBg]     = layer03;
    style->Colors[ImGuiCol_TableRowBgAlt]  = layerAccent03;
    style->Colors[ImGuiCol_TextSelectedBg] = layer03;
    style->Colors[ImGuiCol_TableHeaderBg]  = layer03;
    style->Colors[ImGuiCol_TextSelectedBg] = layer03;

    style->Colors[ImGuiCol_TitleBgActive]    = background;
    style->Colors[ImGuiCol_TitleBgCollapsed] = background;
    style->Colors[ImGuiCol_FrameBgHovered]   = backgroundHover;
    style->Colors[ImGuiCol_FrameBgActive]    = backgroundActive;

    style->Colors[ImGuiCol_NavWindowingDimBg] = background;
    style->Colors[ImGuiCol_ModalWindowDimBg]  = background;

    style->Colors[ImGuiCol_Border]       = background;
    style->Colors[ImGuiCol_BorderShadow] = shadow;

    style->Colors[ImGuiCol_ScrollbarGrab]        = interactive;
    style->Colors[ImGuiCol_ScrollbarGrabHovered] = layerHover03;
    style->Colors[ImGuiCol_ScrollbarGrabActive]  = layerSelected03;

    style->Colors[ImGuiCol_CheckMark]        = interactive;
    style->Colors[ImGuiCol_SliderGrab]       = interactive;
    style->Colors[ImGuiCol_SliderGrabActive] = focus;

    style->Colors[ImGuiCol_Button]        = interactive;
    style->Colors[ImGuiCol_ButtonHovered] = highlight;
    style->Colors[ImGuiCol_ButtonActive]  = focusInverse;

    style->Colors[ImGuiCol_Header]        = background;
    style->Colors[ImGuiCol_HeaderHovered] = backgroundHover;
    style->Colors[ImGuiCol_HeaderActive]  = backgroundActive;

    style->Colors[ImGuiCol_ResizeGrip]        = background;
    style->Colors[ImGuiCol_ResizeGripHovered] = backgroundHover;
    style->Colors[ImGuiCol_ResizeGripActive]  = backgroundActive;

    style->Colors[ImGuiCol_PlotLines]            = missingColorColor;
    style->Colors[ImGuiCol_PlotLinesHovered]     = missingColorColor;
    style->Colors[ImGuiCol_PlotHistogram]        = missingColorColor;
    style->Colors[ImGuiCol_PlotHistogramHovered] = missingColorColor;

    style->Colors[ImGuiCol_Separator]        = background;
    style->Colors[ImGuiCol_SeparatorHovered] = backgroundHover;
    style->Colors[ImGuiCol_SeparatorActive]  = backgroundActive;

    style->Colors[ImGuiCol_Tab]        = layer01;
    style->Colors[ImGuiCol_TabHovered] = layerHover01;

    style->Colors[ImGuiCol_TabActive] = layer02;

    style->Colors[ImGuiCol_TabUnfocused]       = style->Colors[ImGuiCol_Tab];
    style->Colors[ImGuiCol_TabUnfocusedActive] = style->Colors[ImGuiCol_TabActive];

    style->Colors[ImGuiCol_TableBorderStrong] = layerAccent02;
    style->Colors[ImGuiCol_TableBorderLight]  = layerAccent01;

    style->Colors[ImGuiCol_DockingPreview] = interactive;
    style->Colors[ImGuiCol_DragDropTarget] = interactive;

    style->Colors[ImGuiCol_NavHighlight]          = highlight;
    style->Colors[ImGuiCol_NavWindowingHighlight] = highlight;

    // Load Fonts
    const float baseFontSize = 18.0f;
    const float iconFontSize = baseFontSize * 2.0f / 3.0f;  // FontAwesome fonts need to have their sizes reduced
                                                            // by 2.0f/3.0f in order to align correctly

    static const ImWchar fontAwesomeIconRanges[] = {ICON_MIN_FA, ICON_MAX_16_FA, 0};
    static const ImWchar fontAudioIconRanges[]   = {ICON_MIN_FAD, ICON_MAX_16_FAD, 0};

    ImFontConfig icons_config;
    icons_config.FontDataOwnedByAtlas = false;
    icons_config.MergeMode            = true;
    icons_config.PixelSnapH           = true;
    icons_config.GlyphMinAdvanceX     = iconFontSize;

    const cmrc::embedded_filesystem embeddedfilesystem = cmrc::sbk::Fonts::get_filesystem();

    const cmrc::file mainFontFile        = embeddedfilesystem.open("Montserrat-Light.ttf");
    const cmrc::file audioFontFile       = embeddedfilesystem.open("fontaudio/font/" FONT_ICON_FILE_NAME_FAD);
    const cmrc::file fontAwesomeFontFile = embeddedfilesystem.open("Font-Awesome/webfonts/" FONT_ICON_FILE_NAME_FAR);

    assert(mainFontFile.size() > 0);

    ImFontConfig fontConfig;
    fontConfig.FontDataOwnedByAtlas = false;  // the memory is statically owned by the virtual filesystem

    ImFont* mainFont =
        io.Fonts->AddFontFromMemoryTTF((void*)mainFontFile.begin(), mainFontFile.size(), baseFontSize, &fontConfig);

    ImFont* fontAudio = io.Fonts->AddFontFromMemoryTTF((void*)audioFontFile.begin(), audioFontFile.size(),
                                                       iconFontSize * 1.3f, &icons_config, fontAudioIconRanges);

    io.Fonts->AddFontFromMemoryTTF((void*)mainFontFile.begin(), mainFontFile.size(), baseFontSize, &fontConfig);

    ImFont* fontAwesome = io.Fonts->AddFontFromMemoryTTF((void*)fontAwesomeFontFile.begin(), fontAwesomeFontFile.size(),
                                                         iconFontSize, &icons_config, fontAwesomeIconRanges);

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
