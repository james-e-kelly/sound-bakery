#include "review_app.h"

#include "managers/workspace_manager.h"
#include "widgets/review_root_widget.h"

auto create_application() -> gluten::app* 
{
	return new review_app();
}

auto review_app::pre_init() -> void
{
    set_application_display_title("Sound Proof");

    if (std::shared_ptr<gluten::widget_subsystem> widgetSubsystem = get_subsystem_by_class<gluten::widget_subsystem>())
    {
        if (std::shared_ptr<review_root_widget> rootWidget = widgetSubsystem->add_widget_class<review_root_widget>())
        {
            widgetSubsystem->set_root_widget(rootWidget);
        }
    }

    m_workspaceManager = add_manager_class<workspace_manager>();
}

auto review_app::post_init() -> void
{
    if (std::shared_ptr<gluten::renderer_subsystem> renderedSubsystem =
            get_subsystem_by_class<gluten::renderer_subsystem>())
    {
        renderedSubsystem->set_maximised();
    }
}