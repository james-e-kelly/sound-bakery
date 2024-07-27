#include "app_manager.h"

#include "sound_bakery/editor/project/project.h"

#include "app/app.h"
#include "gluten/subsystems/widget_subsystem.h"
#include "widgets/splash_widget.h"
#include "imgui.h"
#include "nfd.h"  // native file dialog

#include "yaml-cpp/yaml.h"

void app_manager::init(gluten::app* app)
{
    app->set_application_display_title(SBK_PRODUCT_NAME);

    splash_widget* splashWidget = app->get_subsystem_by_class<gluten::widget_subsystem>()
                                     ->add_widget_class<splash_widget>();

    splashWidget->show_splash_screen();
}

void app_manager::create_new_project()
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

void app_manager::open_project()
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