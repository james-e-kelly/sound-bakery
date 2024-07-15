#include "App.h"

#include "gluten/subsystems/widget_subsystem.h"
#include "sound_bakery/editor/project/project.h"

#include "managers/app_manager.h"
#include "managers/project_manager.h"
#include "widgets/root_widget.h"

namespace PathHelpers
{
    static const char* ResourcesFolder = "Resources";
}

gluten::app* create_application() { return new editor_app(); }

void editor_app::open_project(const std::filesystem::path& project_file)
{
    if (project_manager* projectManager = get_manager_by_class<project_manager>())
    {
        projectManager->exit();
        projectManager->init_project(project_file);
    }
    else
    {
        projectManager = add_manager_class<project_manager>();
        projectManager->init_project(project_file);
    }
}

void editor_app::create_and_open_project(const std::filesystem::directory_entry& projectFolder) 
{
    sbk::editor::project_configuration newProjectConfig(projectFolder, "Sound Bakery Project");

    if (newProjectConfig.is_valid())
    {
        project_manager* projectManager = get_manager_by_class<project_manager>();

        if (projectManager != nullptr)
        {
            projectManager->exit();
        }
        else
        {
            projectManager = add_manager_class<project_manager>();
        }

        projectManager->init_project(newProjectConfig.project_file());
    }
}

void editor_app::post_init() 
{
    gluten::widget_subsystem* const widgetSubsystem = get_subsystem_by_class<gluten::widget_subsystem>();

    root_widget* const rootWidget = widgetSubsystem->add_widget_class<root_widget>();
    widgetSubsystem->set_root_widget(rootWidget);

    add_manager_class<app_manager>();
}