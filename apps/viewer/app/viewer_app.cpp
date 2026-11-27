#include "viewer_app.h"

#include "gluten/subsystems/audio_subsystem.h"
#include "gluten/subsystems/renderer_subsystem.h"
#include "platform/directory_listing.h"
#include "platform/shell_integration.h"
#include "widgets/viewer_root_widget.h"
#include "widgets/viewer_widget.h"

namespace viewer_app_cli_arguments
{
    static constexpr const char* s_files = "files";
}  // namespace viewer_app_cli_arguments

namespace
{
    constexpr int s_startWindowWidth  = 600;
    constexpr int s_startWindowHeight = 960;

    /**
     * @brief Paths passed on the command line, read before gluten parses it so a launch can be forwarded without opening a window.
     */
    auto get_command_line_files() -> std::vector<std::filesystem::path>
    {
        std::vector<std::filesystem::path> files;
#if defined(_WIN32)
        for (int index = 1; index < __argc; ++index)
        {
            const std::string_view argument = __argv[index];
            if (!argument.starts_with('-'))
            {
                files.push_back(utf8_to_path(argument));
            }
        }
#endif
        return files;
    }
}  // namespace

auto create_application() -> gluten::app*
{
    if (shell_integration::forward_to_running_instance(get_command_line_files()))
    {
        return nullptr;
    }

    return new viewer_app();
}

auto viewer_app::get() -> viewer_app*
{
    return static_cast<viewer_app*>(gluten::app::get());
}

auto viewer_app::cli_setup(boost::program_options::options_description& options) -> void
{
    options.add_options()(viewer_app_cli_arguments::s_files, boost::program_options::value<std::vector<std::string>>(), "audio files or folders to open");
}

auto viewer_app::cli_setup_positional(boost::program_options::positional_options_description& positionalOptions) -> void
{
    positionalOptions.add(viewer_app_cli_arguments::s_files, -1);
}

auto viewer_app::pre_init(const boost::program_options::variables_map& cliVariables) -> void
{
    set_application_display_title("Sound Viewer");

    if (cliVariables.count(viewer_app_cli_arguments::s_files))
    {
        for (const std::string& file : cliVariables.at(viewer_app_cli_arguments::s_files).as<std::vector<std::string>>())
        {
            m_launchFiles.push_back(utf8_to_path(file));
        }
    }

    if (std::shared_ptr<gluten::renderer_subsystem> rendererSubsystem = get_subsystem_by_class<gluten::renderer_subsystem>())
    {
        rendererSubsystem->m_startWindowWidth  = s_startWindowWidth;
        rendererSubsystem->m_startWindowHeight = s_startWindowHeight;
    }

    if (std::shared_ptr<gluten::audio_subsystem> audioSubsystem = add_unique_subsystem_class<gluten::audio_subsystem>())
    {
        // Browsing auditions file after file, often over the network, so start playback without decoding the whole file first.
        audioSubsystem->set_sound_load_mode(SC_SOUND_MODE_STREAM);
    }

    if (std::shared_ptr<gluten::widget_subsystem> widgetSubsystem = get_subsystem_by_class<gluten::widget_subsystem>())
    {
        if (std::shared_ptr<viewer_root_widget> rootWidget = widgetSubsystem->add_widget_class<viewer_root_widget>())
        {
            widgetSubsystem->set_root_widget(rootWidget);
        }
    }
}

auto viewer_app::post_init() -> void
{
    std::shared_ptr<gluten::widget_subsystem> widgetSubsystem = get_subsystem_by_class<gluten::widget_subsystem>();
    if (!widgetSubsystem)
    {
        return;
    }

    m_viewerWidget = widgetSubsystem->add_widget_class_to_root<viewer_widget>(false);

    // Docking needs a frame in progress, so it waits for the first tick.
    get_tick_executor()->post(
        [widgetSubsystem]
        {
            gluten::dockspace_refresh refresh = widgetSubsystem->get_root_widget()->set_manual_layout();
            refresh.assign_widget_to_node(rttr::type::get<viewer_widget>(), refresh.dockspaceID);
        });

    m_viewerWidget->open_path(m_launchFiles.empty() ? std::filesystem::path() : m_launchFiles.front());

    shell_integration::start_accepting_forwarded_launches(
        [this](std::vector<std::filesystem::path> files)
        {
            if (!files.empty())
            {
                m_viewerWidget->open_path(files.front());
            }
            if (std::shared_ptr<gluten::renderer_subsystem> rendererSubsystem = get_subsystem_by_class<gluten::renderer_subsystem>())
            {
                rendererSubsystem->bring_to_front();
            }
        });
}

auto viewer_app::exit() -> void
{
    shell_integration::stop_accepting_forwarded_launches();
    m_viewerWidget.reset();
}
