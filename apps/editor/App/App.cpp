#include "App.h"

#include "sound_bakery/editor/project/project.h"

#include "managers/app_manager.h"
#include "managers/project_manager.h"

namespace PathHelpers
{
    static const char* ResourcesFolder = "Resources";
}

gluten::app* CreateApplication() { return new editor_app(); }

void editor_app::open_project(const std::filesystem::path& project_file)
{
    if (ProjectManager* projectManager = get_manager_by_class<ProjectManager>())
    {
        projectManager->exit();
        projectManager->init_project(project_file);
    }
    else
    {
        projectManager = add_manager_class<ProjectManager>();
        projectManager->init_project(project_file);
    }
}

void editor_app::create_and_open_project(const std::filesystem::directory_entry& projectFolder) 
{
    try
    {
        sbk::editor::project_configuration newProjectConfig(projectFolder, "Sound Bakery Project");

        ProjectManager* projectManager = get_manager_by_class<ProjectManager>();
        
        if (projectManager != nullptr)
        {
            projectManager->exit();
        }
        else
        {
            projectManager = add_manager_class<ProjectManager>();
        }

        projectManager->init_project(newProjectConfig.project_file());
    } 
    catch(std::exception& exception) {}
}