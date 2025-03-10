#include "app_manager.h"

#include "app/app.h"
#include "gluten/subsystems/widget_subsystem.h"
#include "imgui.h"
#include "nfd.h"  // native file dialog
#include "sound_bakery/editor/project/project.h"

void app_manager::init(gluten::app* app)
{
    app->set_application_display_title(SBK_PRODUCT_NAME " " SBK_VERSION_STRING);

    if (app)
    {
        if (const auto widgetSubsystem = app->get_subsystem_by_class<gluten::widget_subsystem>())
        {
            m_splashWidget     = widgetSubsystem->add_widget_class<splash_widget>();
            m_newProjectWidget = widgetSubsystem->add_widget_class<new_project_widget>();

            m_splashWidget->show_splash_screen();

            m_newProjectWidget->set_visibile(false);
        }
    }
}

void app_manager::create_new_project()
{
    m_newProjectWidget->open_new_project_popup();
}

void app_manager::open_project()
{
    NFD_Init();

    nfdchar_t* outPath = NULL;
retry:
    nfdu8filteritem_t filter{.name = "Project", .spec = sbk::editor::project_configuration::projectExtension.data()};
    nfdresult_t result = NFD_OpenDialog(&outPath, &filter, 1, NULL);

    if (result == NFD_ERROR)
        goto retry;
    if (result == NFD_CANCEL)
        return;

    assert(std::filesystem::exists(outPath));

    const std::filesystem::path project_file(outPath);

    static_cast<editor_app*>(get_app())->open_project(project_file);

    NFD_FreePath(outPath);
    NFD_Quit();
}