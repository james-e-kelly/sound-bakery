#include "viewer_app.h"

#include "widgets/viewer_root_widget.h"

auto create_application() -> gluten::app*
{
    return new viewer_app();
}

auto viewer_app::cli_setup(boost::program_options::options_description& options) -> void
{
}

auto viewer_app::pre_init(const boost::program_options::variables_map& cliVariables) -> void
{
    set_application_display_title("Sound Viewer");

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
}

auto viewer_app::exit() -> void
{
}

auto viewer_app::tick_implementation() -> void
{
}