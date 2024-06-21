#include "App.h"

#include "Managers/AppManager.h"
#include "Managers/ProjectManager.h"

namespace PathHelpers
{
    static const char* ResourcesFolder = "Resources";
}

gluten::app* CreateApplication() { return new editor_app(); }

void editor_app::OpenProject(const std::filesystem::path& projectFile)
{
    if (ProjectManager* projectManager = get_manager_by_class<ProjectManager>())
    {
        projectManager->exit();
        projectManager->init_project(projectFile);
    }
    else
    {
        projectManager = add_manager_class<ProjectManager>();
        projectManager->init_project(projectFile);
    }
}
