#include "app.h"

#include "IconsFontAwesome6.h"
#include "IconsFontaudio.h"
#include "subsystems/renderer_subsystem.h"
#include "subsystems/widget_subsystem.h"

#include <cmrc/cmrc.hpp>

CMRC_DECLARE(sbk::Fonts);

namespace PathHelpers
{
    static const char* ResourcesFolder = "Resources";
}

using namespace gluten;

static gluten::app* s_app = nullptr;

gluten::app* gluten::app::get() { return s_app; }

int gluten::app::run(int argc, char** argv)
{
    s_app = this;

    m_executableLocation = std::string(argv[0]);

    add_subsystem_class<renderer_subsystem>();
    add_subsystem_class<widget_subsystem>();

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

    post_init();

    m_currentTime  = std::chrono::high_resolution_clock::now();
    m_previousTime = std::chrono::high_resolution_clock::now();

    m_hasInit = true;

    // Tick
    while (!m_isRequestingExit)
    {
        m_currentTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> timeDiff =
            std::chrono::duration_cast<std::chrono::duration<double>>(m_currentTime - m_previousTime);
        m_previousTime = m_currentTime;

        double deltaTime = timeDiff.count();

        // PreTick
        for (std::shared_ptr<subsystem>& subsystem : m_subsystems)
        {
            subsystem->pre_tick(deltaTime);
        }

        if (m_isRequestingExit)
        {
            break;
        }

        // Tick
        for (std::shared_ptr<subsystem>& subsystem : m_subsystems)
        {
            subsystem->tick(deltaTime);
        }

        for (auto& manager : m_managers)
        {
            manager->tick(deltaTime);
        }

        // Rendering
        for (std::shared_ptr<subsystem>& subsystem : m_subsystems)
        {
            subsystem->tick_rendering(deltaTime);
        }

        FrameMark;
    }

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

void gluten::app::load_fonts()
{
    // Load Fonts
    const float baseFontSize = 18.0f;
    const float iconFontSize = baseFontSize * 2.0f / 3.0f;  // FontAwesome fonts need to have their sizes reduced
                                                            // by 2.0f/3.0f in order to align correctly

    static const ImWchar fontAwesomeIconRanges[] = {ICON_MIN_FA, ICON_MAX_16_FA, 0};
    static const ImWchar fontAudioIconRanges[]   = {ICON_MIN_FAD, ICON_MAX_16_FAD, 0};

    const cmrc::embedded_filesystem embeddedfilesystem = cmrc::sbk::Fonts::get_filesystem();

    const cmrc::file lightFontFile       = embeddedfilesystem.open("Montserrat-Light.ttf");
    const cmrc::file mainFontFile        = embeddedfilesystem.open("Montserrat-Regular.ttf");
    const cmrc::file titleFontFile       = embeddedfilesystem.open("Montserrat-Black.ttf");
    const cmrc::file audioFontFile       = embeddedfilesystem.open("fontaudio/font/" FONT_ICON_FILE_NAME_FAD);
    const cmrc::file fontAwesomeFontFile = embeddedfilesystem.open("Font-Awesome/webfonts/" FONT_ICON_FILE_NAME_FAR);

    assert(mainFontFile.size() > 0);

    ImFontConfig iconFontsConfig;
    iconFontsConfig.FontDataOwnedByAtlas = false;
    iconFontsConfig.MergeMode            = true;
    iconFontsConfig.PixelSnapH           = true;
    iconFontsConfig.GlyphMinAdvanceX     = iconFontSize;
    iconFontsConfig.RasterizerDensity    = 2.0f;

    ImFontConfig fontConfig;
    fontConfig.FontDataOwnedByAtlas = false;  // the memory is statically owned by the virtual filesystem
    fontConfig.RasterizerDensity    = 2.0f;

    ImGuiIO& io = ImGui::GetIO();

    m_fonts[fonts::regular] =
        io.Fonts->AddFontFromMemoryTTF((void*)mainFontFile.begin(), mainFontFile.size(), baseFontSize, &fontConfig);

    m_fonts[fonts::regular_audio_icons] =
        io.Fonts->AddFontFromMemoryTTF((void*)mainFontFile.begin(), mainFontFile.size(), baseFontSize, &fontConfig);
    io.Fonts->AddFontFromMemoryTTF((void*)audioFontFile.begin(), audioFontFile.size(), iconFontSize * 1.3f,
                                   &iconFontsConfig, fontAudioIconRanges);

    m_fonts[fonts::regular_font_awesome] =
        io.Fonts->AddFontFromMemoryTTF((void*)mainFontFile.begin(), mainFontFile.size(), baseFontSize, &fontConfig);
    io.Fonts->AddFontFromMemoryTTF((void*)fontAwesomeFontFile.begin(), fontAwesomeFontFile.size(), iconFontSize,
                                   &iconFontsConfig, fontAwesomeIconRanges);

    m_fonts[fonts::light] =
        io.Fonts->AddFontFromMemoryTTF((void*)lightFontFile.begin(), lightFontFile.size(), baseFontSize, &fontConfig);
    m_fonts[fonts::title] = io.Fonts->AddFontFromMemoryTTF((void*)titleFontFile.begin(), titleFontFile.size(),
                                                           baseFontSize * 1.2f, &fontConfig);
}

void gluten::app::request_exit() { m_isRequestingExit = true; }

bool gluten::app::is_maximized() { return get_subsystem_by_class<gluten::renderer_subsystem>()->is_maximized(); }

void gluten::app::set_application_display_title(const std::string& title)
{
    m_applicationDisplayTitle = title;
    get_subsystem_by_class<renderer_subsystem>()->set_window_title(title);
}
