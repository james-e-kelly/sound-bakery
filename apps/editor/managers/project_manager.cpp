#include "project_manager.h"

#include "App/App.h"
#include "gluten/subsystems/widget_subsystem.h"
#include "widgets/details_widget.h"
#include "widgets/play_controls_widget.h"
#include "widgets/project_explorer_widget.h"

#include "sound_bakery/system.h"
#include "sound_bakery/core/database/database.h"
#include "sound_bakery/core/object/object_tracker.h"
#include "sound_bakery/editor/project/project.h"
#include "sound_bakery/factory.h"
#include "sound_bakery/node/container/sound_container.h"
#include "sound_bakery/serialization/serializer.h"
#include "sound_bakery/sound/sound.h"
#include "sound_bakery/soundbank/soundbank.h"
#include "sound_bakery/system.h"
#include "sound_bakery/util/type_helper.h"

void ProjectManager::init_project(const std::filesystem::path& project_file)
{
    sbk::engine::system::open_project(project_file);

    get_app()
        ->get_subsystem_by_class<gluten::widget_subsystem>()
        ->add_widget_class_to_root<ProjectExplorerWidget>();
    get_app()
        ->get_subsystem_by_class<gluten::widget_subsystem>()
        ->add_widget_class_to_root<PlayerWidget>();
    get_app()
        ->get_subsystem_by_class<gluten::widget_subsystem>()
        ->add_widget_class_to_root<DetailsWidget>();
}

void ProjectManager::tick(double deltaTime)
{
    sbk::engine::system::update();
}

void ProjectManager::exit() 
{ 
    sbk::engine::system::destroy(); 
}

void ProjectManager::SaveProject() const 
{ 
    sbk::engine::system::get()->get_project()->save_project(); 
}

sbk::engine::SoundContainer* ProjectManager::GetPreviewSoundContainer() const
{
    return sbk::engine::system::get_project()->get_preview_container().lock().get();
}