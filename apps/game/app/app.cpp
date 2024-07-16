#include "App.h"

#include "gluten/subsystems/widget_subsystem.h"
#include "sound_bakery/editor/project/project.h"

#include "nfd.h"

#include "widgets/root_widget.h"

namespace PathHelpers
{
    static const char* ResourcesFolder = "Resources";
}

gluten::app* create_application() { return new game_app(); }

void game_app::post_init() 
{
    gluten::widget_subsystem* const widgetSubsystem = get_subsystem_by_class<gluten::widget_subsystem>();

    root_widget* const rootWidget = widgetSubsystem->add_widget_class<root_widget>();
    widgetSubsystem->set_root_widget(rootWidget);
}

void game_app::open_soundbank()
{
    nfdchar_t* outPath = NULL;
retry:
    nfdresult_t result = NFD_OpenDialog(sbk::editor::project_configuration::outputBankExtension.data(), NULL, &outPath);

    if (result == NFD_ERROR)
        goto retry;
    if (result == NFD_CANCEL)
        return;

    assert(std::filesystem::exists(outPath));


}