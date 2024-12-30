#include "app.h"

#include "gluten/subsystems/widget_subsystem.h"
#include "managers/app_manager.h"
#include "managers/project_manager.h"
#include "sound_bakery/editor/project/project.h"
#include "widgets/root_widget.h"

namespace PathHelpers
{
    static const char* ResourcesFolder = "Resources";
}

gluten::app* create_application() { return new editor_app(); }

void editor_app::open_project(const std::filesystem::path& project_file)
{
    remove_manager_by_class<project_manager>();
    if (std::shared_ptr<project_manager> projectManager = add_manager_class<project_manager>())
    {
        projectManager->init_project(project_file);
    }
}

void editor_app::create_and_open_project(const std::filesystem::directory_entry& projectFolder)
{
    remove_manager_by_class<project_manager>();
    if (std::shared_ptr<project_manager> projectManager = add_manager_class<project_manager>())
    {
        projectManager->create_project(projectFolder, "Sound Bakery Project");
    }
}

void editor_app::post_init()
{
    std::shared_ptr<gluten::widget_subsystem> widgetSubsystem = get_subsystem_by_class<gluten::widget_subsystem>();

    std::shared_ptr<root_widget> rootWidget = widgetSubsystem->add_widget_class<root_widget>();
    widgetSubsystem->set_root_widget(rootWidget.get());

    add_manager_class<app_manager>();
}