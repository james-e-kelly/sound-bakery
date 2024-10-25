#include "app.h"

#include "gluten/subsystems/widget_subsystem.h"
#include "sound_bakery/editor/project/project.h"

#include "nfd.h"

#include "widgets/root_widget.h"
#include "widgets/soundbank_viewer.h"

namespace PathHelpers
{
    static const char* ResourcesFolder = "Resources";
}

gluten::app* create_application() { return new game_app(); }

void game_app::post_init() 
{
    std::shared_ptr<gluten::widget_subsystem> widgetSubsystem = get_subsystem_by_class<gluten::widget_subsystem>();

    std::shared_ptr<root_widget> rootWidget = widgetSubsystem->add_widget_class<root_widget>();
    widgetSubsystem->set_root_widget(rootWidget.get());
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

    m_soundbankViewerWidget.reset();
    m_soundbankViewerWidget = get_subsystem_by_class<gluten::widget_subsystem>()->add_widget_class_to_root<soundbank_viewer_widget>(false);
    m_soundbankViewerWidget->set_soundbank_to_view(outPath);
}