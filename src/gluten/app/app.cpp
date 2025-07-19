#include "app.h"

#include "IconsFontAwesome6.h"
#include "IconsFontaudio.h"
#include "IconsLucide.h"
#include "subsystems/renderer_subsystem.h"
#include "subsystems/widget_subsystem.h"
#include "nfd.h"
//#include "Fontawesome"

#include <cmrc/cmrc.hpp>

CMRC_DECLARE(sbk::fonts);

static gluten::app* s_app = nullptr;

gluten::app* gluten::app::get() { return s_app; }

int gluten::app::run(int argc, char** argv)
{
    s_app = this;

    m_executableLocation = std::string(argv[0]);

    add_subsystem_class<renderer_subsystem>();
    add_subsystem_class<widget_subsystem>();

    m_tickExecutor = make_manual_executor();

    pre_init();

    // PreInit
    for (std::shared_ptr<subsystem>& subsystem : m_subsystems)
    {
        if (int errorCode = subsystem->pre_init(argc, argv); errorCode != 0)
        {
            return errorCode;
        }
    }

    // Init
    for (std::shared_ptr<subsystem>& subsystem : m_subsystems)
    {
        if (int errorCode = subsystem->init(); errorCode != 0)
        {
            return errorCode;
        }
    }

    load_fonts();

    m_currentTime  = std::chrono::high_resolution_clock::now();
    m_previousTime = std::chrono::high_resolution_clock::now();

    m_hasInit = true;

    post_init();

    start();

    while (!m_isRequestingExit)
    {
        tick();
    }

    exit();

    for (auto& manager : m_managers)
    {
        manager->exit();
    }

    for (std::shared_ptr<subsystem>& subsystem : m_subsystems)
    {
        subsystem->exit();
    }

    return 0;
}

auto gluten::app::start() -> void
{
    tick_begin();
    for (std::shared_ptr<subsystem>& subsystem : m_subsystems)
    {
        subsystem->start();
    }

    for (std::shared_ptr<manager>& manager : m_managers)
    {
        manager->start();
    }
    tick_end();
}

auto gluten::app::tick() -> void
{
    tick_begin();
    m_tickExecutor->loop(m_tickExecutor->size());
    tick_implementation();
    tick_end(); 
}

auto gluten::app::tick_begin() -> void
{
    m_currentTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> timeDiff =
        std::chrono::duration_cast<std::chrono::duration<double>>(m_currentTime - m_previousTime);
    m_previousTime = m_currentTime;

    m_deltaTime = timeDiff.count();

    {
        ZoneScopedN("PreTick");
        for (std::shared_ptr<subsystem>& subsystem : m_subsystems)
        {
            subsystem->pre_tick(m_deltaTime);
        }
    }

    if (m_isRequestingExit)
    {
        return;
    }

    {
        ZoneScopedN("SubsystemTick");
        for (std::shared_ptr<subsystem>& subsystem : m_subsystems)
        {
            subsystem->tick(m_deltaTime);
        }
    }

    {
        ZoneScopedN("ManagerTick");
        for (auto& manager : m_managers)
        {
            manager->tick(m_deltaTime);
        }
    }
}

auto gluten::app::tick_end() -> void
{
    {
        ZoneScopedN("RenderingTick");
        for (std::shared_ptr<subsystem>& subsystem : m_subsystems)
        {
            subsystem->tick_rendering(m_deltaTime);
        }
    }

    FrameMark;
}

void gluten::app::load_fonts()
{
    static const ImWchar fontAwesomeIconRanges[] = {ICON_MIN_FA, ICON_MAX_16_FA, 0};
    static const ImWchar fontAudioIconRanges[]   = {ICON_MIN_FAD, ICON_MAX_16_FAD, 0};
    static const ImWchar lucideIconRanges[]      = {ICON_MIN_LC, ICON_MAX_16_LC, 0};

    const cmrc::embedded_filesystem embeddedfilesystem = cmrc::sbk::fonts::get_filesystem();

    const cmrc::file lightFontFile       = embeddedfilesystem.open("Montserrat-Light.ttf");
    const cmrc::file mainFontFile        = embeddedfilesystem.open("Montserrat-Regular.ttf");
    const cmrc::file titleFontFile       = embeddedfilesystem.open("Montserrat-Black.ttf");
    const cmrc::file audioFontFile       = embeddedfilesystem.open("fontaudio/font/" FONT_ICON_FILE_NAME_FAD);
    const cmrc::file fontAwesomeFontFile = embeddedfilesystem.open("Font-Awesome/webfonts/" FONT_ICON_FILE_NAME_FAS);
    const cmrc::file lucideFontFile      = embeddedfilesystem.open("Lucide.ttf");

    assert(mainFontFile.size() > 0);

    ImFontConfig iconFontsConfig;
    iconFontsConfig.FontDataOwnedByAtlas = false;
    iconFontsConfig.MergeMode            = true;
    iconFontsConfig.PixelSnapH           = true;
    iconFontsConfig.GlyphMinAdvanceX     = g_baseIconFontSize;
    iconFontsConfig.RasterizerDensity    = 1.0f;

    ImFontConfig fontConfig;
    fontConfig.FontDataOwnedByAtlas = false;  // the memory is statically owned by the virtual filesystem
    fontConfig.RasterizerDensity    = 1.0f;

    ImGuiIO& io = ImGui::GetIO();

    m_fonts[fonts::regular] =
        io.Fonts->AddFontFromMemoryTTF((void*)mainFontFile.begin(), mainFontFile.size(), g_baseFontSize, &fontConfig);

    m_fonts[fonts::regular_audio_icons] =
        io.Fonts->AddFontFromMemoryTTF((void*)mainFontFile.begin(), mainFontFile.size(), g_baseFontSize, &fontConfig);
    io.Fonts->AddFontFromMemoryTTF((void*)audioFontFile.begin(), audioFontFile.size(), g_baseIconFontSize * 1.3f,
                                   &iconFontsConfig, fontAudioIconRanges);

    m_fonts[fonts::regular_lucide_icons] =
        io.Fonts->AddFontFromMemoryTTF((void*)mainFontFile.begin(), mainFontFile.size(), g_baseFontSize, &fontConfig);
    io.Fonts->AddFontFromMemoryTTF((void*)lucideFontFile.begin(), lucideFontFile.size(), g_baseIconFontSize * 1.3f,
                                   &iconFontsConfig, lucideIconRanges);

    m_fonts[fonts::regular_font_awesome] =
        io.Fonts->AddFontFromMemoryTTF((void*)mainFontFile.begin(), mainFontFile.size(), g_baseFontSize, &fontConfig);
    io.Fonts->AddFontFromMemoryTTF((void*)fontAwesomeFontFile.begin(), fontAwesomeFontFile.size(), g_baseIconFontSize,
                                   &iconFontsConfig, fontAwesomeIconRanges);

    m_fonts[fonts::light] =
        io.Fonts->AddFontFromMemoryTTF((void*)lightFontFile.begin(), lightFontFile.size(), g_baseFontSize, &fontConfig);
    m_fonts[fonts::title] = io.Fonts->AddFontFromMemoryTTF((void*)titleFontFile.begin(), titleFontFile.size(),
                                                           g_baseFontSize * 1.2f, &fontConfig);

    m_fonts[fonts::title_lucide_icons] =
        io.Fonts->AddFontFromMemoryTTF((void*)titleFontFile.begin(), titleFontFile.size(), g_baseFontSize * 1.2f, &fontConfig);
    io.Fonts->AddFontFromMemoryTTF((void*)lucideFontFile.begin(), lucideFontFile.size(), g_baseIconFontSize * 1.3f,
                                   &iconFontsConfig, lucideIconRanges);
}

void gluten::app::request_exit() { m_isRequestingExit = true; }

bool gluten::app::is_maximized() { return get_subsystem_by_class<gluten::renderer_subsystem>()->is_maximized(); }

void gluten::app::set_application_display_title(const std::string& title)
{
    m_applicationDisplayTitle = title;
    get_subsystem_by_class<renderer_subsystem>()->set_window_title(title);
}

auto gluten::app::open_select_folder_dialog() -> std::filesystem::path
{
    NFD_Init();

    std::filesystem::path result;
    nfdchar_t* outPath = NULL;

retry:
    nfdresult_t pickFolderResult = NFD_PickFolder(&outPath, std::filesystem::current_path().string().c_str());

    switch (pickFolderResult)
    {
        case NFD_OKAY:
            result              = outPath;
            break;

        case NFD_CANCEL:
            result.clear();
            break;

        case NFD_ERROR:
        default:
            goto retry;
            break;
    }

    NFD_FreePath(outPath);
    NFD_Quit();

    return result;
}

auto gluten::app::open_select_file_dialog(const std::string& name, const std::string& fileExtensionNoDots) -> std::filesystem::path
{
    std::filesystem::path result;

    NFD_Init();

    nfdchar_t* outPath = NULL;
retry:
    nfdu8filteritem_t filter{.name = name.c_str(), .spec = fileExtensionNoDots.c_str()};
    nfdresult_t openResult = NFD_OpenDialog(&outPath, &filter, 1, NULL);

    switch (openResult)
    {
        case NFD_OKAY:
            result = outPath;
            break;

        case NFD_CANCEL:
            result.clear();
            break;

        case NFD_ERROR:
        default:
            goto retry;
            break;
    }

    assert(std::filesystem::exists(outPath));

    result = outPath;

    NFD_FreePath(outPath);
    NFD_Quit();

    return result;
}