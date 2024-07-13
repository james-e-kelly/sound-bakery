#include "app_manager.h"

#include "sound_bakery/editor/project/project.h"

#include "app/app.h"
#include "gluten/subsystems/widget_subsystem.h"
#include "widgets/splash_widget.h"
#include "imgui.h"
#include "nfd.h"  // native file dialog

#include "yaml-cpp/yaml.h"

void AppManager::init(gluten::app* app)
{
    SplashWidget* splashWidget = app->get_subsystem_by_class<gluten::widget_subsystem>()
                                     ->add_widget_class<SplashWidget>();

    splashWidget->ShowSplashScreen();
}

void AppManager::CreateNewProject()
{
    nfdchar_t* outPath = NULL;

retry:
    nfdresult_t pickFolderResult = NFD_PickFolder(
        std::filesystem::current_path().string().c_str(), &outPath);

    switch (pickFolderResult)
    {
        case NFD_OKAY:
            break;

        case NFD_CANCEL:
            return;
            break;

        case NFD_ERROR:
        default:
            goto retry;
            break;
    }

    std::filesystem::directory_entry projectDirectory(outPath);

    if (std::filesystem::exists(projectDirectory))
    {
        static_cast<editor_app*>(get_app())->create_and_open_project(projectDirectory);
    }
}

void AppManager::open_project()
{
    nfdchar_t* outPath = NULL;
retry:
    nfdresult_t result = NFD_OpenDialog(sbk::editor::project_configuration::projectExtension.data(), NULL, &outPath);

    if (result == NFD_ERROR)
        goto retry;
    if (result == NFD_CANCEL)
        return;

    assert(std::filesystem::exists(outPath));

    const std::filesystem::path project_file(outPath);

    static_cast<editor_app*>(get_app())->open_project(project_file);
}