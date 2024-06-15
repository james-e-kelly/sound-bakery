#include "ProjectManager.h"

#include "App/App.h"
#include "Subsystems/WidgetSubsystem.h"
#include "Widgets/DetailsWidget.h"
#include "Widgets/PlayControlsWidget.h"
#include "Widgets/ProjectExplorerWidget.h"

#include "sound_bakery/system.h"
#include "sound_bakery/core/database/database.h"
#include "sound_bakery/core/object/object_global.h"
#include "sound_bakery/core/object/object_tracker.h"
#include "sound_bakery/editor/project/project.h"
#include "sound_bakery/factory.h"
#include "sound_bakery/node/container/sound_container.h"
#include "sound_bakery/serialization/serializer.h"
#include "sound_bakery/sound/sound.h"
#include "sound_bakery/soundbank/soundbank.h"
#include "sound_bakery/system.h"
#include "sound_bakery/util/type_helper.h"

void ProjectManager::Init(const std::filesystem::path& projectFile)
{
    sbk::engine::system::open_project(projectFile);

    GetApp()
        ->get_subsystem_by_class<widget_subsystem>()
        ->add_widget_class_to_root<ProjectExplorerWidget>();
    GetApp()
        ->get_subsystem_by_class<widget_subsystem>()
        ->add_widget_class_to_root<PlayerWidget>();
    GetApp()
        ->get_subsystem_by_class<widget_subsystem>()
        ->add_widget_class_to_root<DetailsWidget>();
}

void ProjectManager::Tick(double deltaTime)
{
    sbk::engine::system::update();
}

void ProjectManager::Exit() 
{ 
    sbk::engine::system::destroy(); 
}

void ProjectManager::SaveProject() const 
{ 
    sbk::engine::system::get()->getProject()->saveProject(); 
}

sbk::engine::SoundContainer* ProjectManager::GetPreviewSoundContainer() const
{
    return sbk::engine::system::getProject()->getPreviewContainer();
}